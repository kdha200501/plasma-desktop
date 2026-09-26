/*
    SPDX-FileCopyrightText: 2026 Quick Look Contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QList>
#include <QPointer>
#include <QUrl>

#include <functional>

/**
 * Thin D-Bus client helper for the Quick Look service (org.kde.quicklook).
 *
 * The service is D-Bus-activated (see the plasma-quicklook systemd unit), so no
 * explicit process spawning is needed - the first call starts it, the same way
 * `krunner <query>` activates KRunner.
 *
 * This helper is deliberately duplicated (static-linked) in the desktop Folder
 * View plugin instead of introducing a cross-repo library dependency: the whole
 * surface is a single D-Bus method call.
 */
class QuickLookClosedWatcher;

class QuickLookClient
{
public:
    /**
     * Sends a live-update of the quick look preview for the given item URLs.
     * Only meaningful (and only applied) while this process is the owner of the
     * shared preview window - see requestPreview(); a live-update from a
     * non-owning process is ignored by the service.
     */
    static void previewUrls(const QList<QUrl> &urls);

    /**
     * Explicitly opens the quick look preview for the given item URLs, taking
     * ownership of the shared preview window for this process, activating the
     * service if it is not running yet.
     *
     * The preview window is shared system-wide (e.g. several Dolphin windows
     * run as separate processes and would otherwise each keep their own
     * "preview is open" belief), so ownership is arbitrated by the service:
     * the caller that last explicitly opened the window is the only one whose
     * live-updates (previewUrls()) are applied while it is visible.
     */
    static void requestPreview(const QList<QUrl> &urls);

    /**
     * Returns the D-Bus unique name of the caller that currently owns the
     * preview, or the empty string if the preview is closed (or was opened
     * locally / same-process, which carries no owner). A non-empty value that
     * is not equal to ownUniqueName() means an open preview is owned by
     * ANOTHER program (e.g. a Dolphin window) - the precondition for the
     * desktop to take the preview over (macOS Quick Look semantics, see
     * retakeQuickLookOnFocus() in the Folder Model).
     *
     * This does a D-Bus round trip to the service; it first checks that the
     * service is already running so it never D-Bus-activates just to peek.
     */
    static QString previewOwner();

    /**
     * Returns this process's unique D-Bus connection name - the identity the
     * service records as the preview owner when this process opens it (see
     * previewOwner()).
     */
    static QString ownUniqueName();

    /**
     * Unconditionally dismisses the shared preview. The service closes the
     * window for an empty URL list - that close is not owner-gated (unlike the
     * close method), so a client that does not currently own the preview can
     * dismiss it, e.g. the desktop regaining the focus with an empty selection
     * after a file manager took the preview over (see
     * FolderModel::retakeQuickLookOnFocus()).
     */
    static void dismissPreview();

    /**
     * Hides the quick look window (best effort; the service may not be running).
     */
    static void closeQuickLook();

    /**
     * Marks \a owner as the viewer inside this process that explicitly opened
     * the quick look preview via requestPreview(). This is the *intra-process*
     * half of the ownership rule - it keeps one window/view from live-updating
     * the preview based on its selection moving while another viewer in the
     * same process (the desktop Folder View has one per screen, all in a single
     * process) owns it. (The *inter-process* half is enforced by the service
     * itself, which only applies live-updates from its current owner; see
     * requestPreview().)
     */
    static void setPreviewOwner(QObject *owner);

    /**
     * Returns true if \a owner currently owns the quick look preview (i.e. it is
     * the most recent caller of setPreviewOwner() and has not been destroyed).
     */
    static bool isPreviewOwner(const QObject *owner);

    /**
     * Invokes \a callback whenever the quick look window is hidden ("closed") while
     * already open. Used to live-update the preview as the file manager's selection
     * moves. The callback is invoked on the current thread.
     *
     * The returned watcher is owned by the caller. When \a owner is destroyed the
     * callback is dropped and the watcher deleted, so it can never be invoked with
     * a dangling `this` - keep the returned value alive for as long as the
     * registration should be active.
     */
    static QPointer<QuickLookClosedWatcher> connectClosed(QObject *owner, std::function<void()> callback);

private:
    static QPointer<QObject> m_previewOwner;
};
