/*
    SPDX-FileCopyrightText: Ken <https://stackoverflow.com/users/1568857/ken>
    SPDX-FileCopyrightText: 2016 Leslie Zhai <xiangzhai83@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "shortcut.h"

#include <KStandardShortcut>

#include <QKeyEvent>

ShortCut::ShortCut(QObject *parent)
    : QObject(parent)
{
}

void ShortCut::installAsEventFilterFor(QObject *target)
{
    if (target) {
        target->installEventFilter(this);
    }
}

bool ShortCut::eventFilter(QObject *obj, QEvent *e)
{

  if (e->type() != QEvent::KeyPress) {
      return QObject::eventFilter(obj, e);
  }

  QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
  bool isControl = keyEvent->modifiers() & Qt::ControlModifier;
  bool isShift = keyEvent->modifiers() & Qt::ShiftModifier;

  if (isControl && isShift && keyEvent->key() == Qt::Key_N) {
      Q_EMIT createFolder();
      return true;
  }

  if (isControl && isShift && keyEvent->key() == Qt::Key_H) {
      Q_EMIT runHome();
      return true;
  }

  if (isControl && keyEvent->key() == Qt::Key_N) {
      Q_EMIT runHome();
      return true;
  }

  // The reactions to the following signals already checks for the presence of selection

  if (isControl && keyEvent->key() == Qt::Key_Down) {
      Q_EMIT open();
      return true;
  }

  if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
      Q_EMIT renameFile();
      return true;
  }

  if (isControl && keyEvent->key() == Qt::Key_D) {
      Q_EMIT duplicate();
      return true;
  }

  if (isControl && keyEvent->key() == Qt::Key_Backspace) {
      Q_EMIT moveToTrash();
      return true;
  }

  if (isControl && keyEvent->key() == Qt::Key_I) {
      Q_EMIT viewProperties();
      return true;
  }

  return QObject::eventFilter(obj, e);
}

#include "moc_shortcut.cpp"
