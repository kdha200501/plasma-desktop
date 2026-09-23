/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

var iconSizes = [Kirigami.Units.iconSizes.smallMedium,
                 Kirigami.Units.iconSizes.medium,
                 Kirigami.Units.iconSizes.large,
                 Kirigami.Units.iconSizes.huge,
                 Kirigami.Units.iconSizes.large*2,
                 Kirigami.Units.iconSizes.enormous,
                 Kirigami.Units.iconSizes.enormous*2];

function iconSizeFromTheme(size) {
    return iconSizes[size];
}

function effectiveNavDirection(flow, layoutDirection, direction) {
    if (direction == Qt.LeftArrow) {
        if (flow == GridView.FlowLeftToRight) {
            if (layoutDirection == Qt.LeftToRight) {
                return Qt.LeftArrow;
            } else {
                return Qt.RightArrow;
            }
        } else {
            if (layoutDirection == Qt.LeftToRight) {
                return Qt.UpArrow;
            } else {
                return Qt.DownArrow;
            }
        }
    } else if (direction == Qt.RightArrow) {
        if (flow == GridView.FlowLeftToRight) {
            if (layoutDirection == Qt.LeftToRight) {
                return Qt.RightArrow;
            } else {
                return Qt.LeftArrow;
            }
        } else {
            if (layoutDirection == Qt.LeftToRight) {
                return Qt.DownArrow;
            } else {
                return Qt.UpArrow;
            }
        }
    } else if (direction == Qt.UpArrow) {
        if (flow == GridView.FlowLeftToRight) {
            return Qt.UpArrow;
        } else {
            return Qt.LeftArrow;
        }
    } else if (direction == Qt.DownArrow) {
        if (flow == GridView.FlowLeftToRight) {
            return Qt.DownArrow;
        } else {
            return Qt.RightArrow
        }
    }
}

function isFileDrag(event) {
    var taskUrl = event.mimeData.formats.indexOf("text/x-orgkdeplasmataskmanager_taskurl") != -1;
    var arkService = event.mimeData.formats.indexOf("application/x-kde-ark-dndextract-service") != -1;
    var arkPath = event.mimeData.formats.indexOf("application/x-kde-ark-dndextract-path") != -1;

    return (event.mimeData.hasUrls || taskUrl || (arkService && arkPath));
}

// Same-device-aware preferred drop action for the hovered drop position, so the
// compositor glyph (negotiated over wl_data_offer.set_actions) matches what the
// drop will actually do. \a pos is in the FolderView's coordinate system.
// Returns Qt::MoveAction, Qt::CopyAction, or -1 to keep Qt's proposed action.
function suggestedDropAction(view, model, pos, urls) {
    if (!view || !model || !urls || urls.length === 0) {
        return -1;
    }

    const gridView = view.view;
    const contentPos = view.mapToItem(gridView.contentItem, pos.x, pos.y);

    let index = -1;
    const item = gridView.safeItemAt(contentPos.x, contentPos.y);
    if (item && !item.blank) {
        index = item.index;
    }

    return model.suggestedDropActionForItem(index, urls);
}
