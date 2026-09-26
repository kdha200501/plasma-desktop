/*
    SPDX-FileCopyrightText: 2026 Quick Look Contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "quicklookclient.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QUrl>

namespace
{
const QString QUICKLOOK_SERVICE = QStringLiteral("org.kde.quicklook");
const QString QUICKLOOK_PATH = QStringLiteral("/App");
const QString QUICKLOOK_INTERFACE = QStringLiteral("org.kde.quicklook.App");
}

QPointer<QObject> QuickLookClient::m_previewOwner;

// Not parented (it must outlive the session-bus connection it holds); its
// lifetime is managed by the caller of QuickLookClient::connectClosed().
class QuickLookClosedWatcher : public QObject
{
    Q_OBJECT
public:
    explicit QuickLookClosedWatcher(QObject *parent = nullptr)
        : QObject(parent)
    {
        // Watch the "closed" signal of the service. The match rule is registered
        // on the session bus, so it also fires after the service is (re)started
        // via D-Bus activation, which always precedes any live update.
        QDBusConnection::sessionBus().connect(QUICKLOOK_SERVICE,
                                              QUICKLOOK_PATH,
                                              QUICKLOOK_INTERFACE,
                                              QStringLiteral("closed"),
                                              this,
                                              SLOT(closed()));
    }

    void setCallback(std::function<void()> callback)
    {
        m_callback = std::move(callback);
    }

    private Q_SLOTS:
    void closed()
    {
        if (m_callback) {
            m_callback();
        }
    }

private:
    std::function<void()> m_callback;
};

void QuickLookClient::setPreviewOwner(QObject *owner)
{
    m_previewOwner = owner;
}

bool QuickLookClient::isPreviewOwner(const QObject *owner)
{
    return !m_previewOwner.isNull() && m_previewOwner.data() == owner;
}

namespace
{
void callPreviewUrls(const QList<QUrl> &urls, bool explicitTrigger, const QString &anchorScreenName)
{
    if (urls.isEmpty()) {
        return;
    }

    QStringList strUrls;
    strUrls.reserve(urls.size());
    for (const QUrl &u : urls) {
        strUrls << u.toString();
    }

    // The service is D-Bus-activated; this call starts it if it is not
    // running. It is blocking only for the (short) activation+method round
    // trip, mirroring how `krunner <query>` activates KRunner. anchorScreenName
    // is the monitor the item is on, so the service anchors the window there.
    QDBusInterface interface(QUICKLOOK_SERVICE, QUICKLOOK_PATH, QUICKLOOK_INTERFACE, QDBusConnection::sessionBus());
    QDBusMessage reply = interface.call(QStringLiteral("previewUrls"), strUrls, explicitTrigger, anchorScreenName);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "Quick Look: could not invoke org.kde.quicklook:" << reply.errorMessage();
    }
}
}

void QuickLookClient::previewUrls(const QList<QUrl> &urls)
{
    callPreviewUrls(urls, false, QString());
}

void QuickLookClient::requestPreview(const QList<QUrl> &urls, const QString &anchorScreenName)
{
    callPreviewUrls(urls, true, anchorScreenName);
}

QString QuickLookClient::previewOwner()
{
    // Only ask the service if it is already registered on the session bus:
    // calling owner() via QDBusInterface is D-Bus-activating, which would start
    // the (hidden) daemon just to answer a question - the retake logic wants
    // a passive peek only. NameHasOwner is called on the bus daemon itself, so
    // it can never trigger activation of the target.
    QDBusMessage hasOwner = QDBusMessage::createMethodCall(
            QStringLiteral("org.freedesktop.DBus"),
            QStringLiteral("/org/freedesktop/DBus"),
            QStringLiteral("org.freedesktop.DBus"),
            QStringLiteral("NameHasOwner"));
    hasOwner << QUICKLOOK_SERVICE;
    QDBusMessage hasOwnerReply = QDBusConnection::sessionBus().call(hasOwner);
    if (hasOwnerReply.type() != QDBusMessage::ReplyMessage
            || hasOwnerReply.arguments().isEmpty()
            || !hasOwnerReply.arguments().first().toBool()) {
        return QString();
    }

    QDBusInterface interface(QUICKLOOK_SERVICE, QUICKLOOK_PATH, QUICKLOOK_INTERFACE, QDBusConnection::sessionBus());
    QDBusMessage reply = interface.call(QStringLiteral("owner"));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        return QString();
    }
    // owner() returns a single s-type argument: the owner's D-Bus unique name,
    // or the empty string while the preview is closed.
    const QVariantList args = reply.arguments();
    if (!args.isEmpty()) {
        return args.first().toString();
    }
    return QString();
}

QString QuickLookClient::ownUniqueName()
{
    // This connection's unique name: the identity the service records as the
    // preview owner for this process. (Calling the bus' Hello again fails
    // with "Hello() already called", so it cannot be used to query it.)
    return QDBusConnection::sessionBus().baseService();
}

void QuickLookClient::dismissPreview()
{
    // An empty URL list makes the service close the window (see
    // QuickLookWindow::previewUrls), and unlike the close method that close is
    // not gated on the caller owning the preview - a non-owner can dismiss it.
    // callPreviewUrls() is bypassed on purpose: it returns early for an empty
    // list, which is exactly the payload we need here.
    QDBusInterface interface(QUICKLOOK_SERVICE, QUICKLOOK_PATH, QUICKLOOK_INTERFACE, QDBusConnection::sessionBus());
    QDBusMessage reply = interface.call(QStringLiteral("previewUrls"), QStringList(), true, QString());
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "Quick Look: could not invoke org.kde.quicklook dismiss:" << reply.errorMessage();
    }
}

void QuickLookClient::closeQuickLook()
{
    QDBusInterface interface(QUICKLOOK_SERVICE, QUICKLOOK_PATH, QUICKLOOK_INTERFACE, QDBusConnection::sessionBus());
    QDBusMessage reply = interface.call(QStringLiteral("close"));
    if (reply.type() == QDBusMessage::ErrorMessage && QDBusError(reply).type() != QDBusError::ServiceUnknown) {
        qWarning() << "Quick Look: could not invoke org.kde.quicklook close:" << reply.errorMessage();
    }
}

QPointer<QuickLookClosedWatcher> QuickLookClient::connectClosed(QObject *owner, std::function<void()> callback)
{
    auto *watcher = new QuickLookClosedWatcher;
    QPointer<QuickLookClosedWatcher> safeWatcher = watcher;
    QObject::connect(owner, &QObject::destroyed, watcher, [safeWatcher] {
        // Drop the callback before the owner is gone, so a "closed" signal
        // during destruction cannot reach its raw `this` capture.
        if (safeWatcher) {
            safeWatcher->setCallback(nullptr);
            delete safeWatcher;
        }
    });
    watcher->setCallback(std::move(callback));
    return watcher;
}

#include "quicklookclient.moc"
