//
// C++ Implementation: selection navigation dialog
//
// Description:
//      Shows keyboard, and allows to input text string
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
// selnavig.cpp

#include "selnavig.h"
#include "cri18n.h"
#if CR_POCKETBOOK == 1
#include "reader_ui.h"

namespace {
const int searchCommands[] = { MCMD_SCROLL_BACK, MCMD_OK, MCMD_SCROLL_FORWARD };

lvRect searchButtonRect(const lvRect &bar, int index)
{
    return lvRect(bar.left + bar.width() * index / 3, bar.top,
                  bar.left + bar.width() * (index + 1) / 3, bar.bottom);
}
}

void CRSelNavigationDialog::reconfigure(int)
{
    lvRect bar = _wm->getScreen()->getRect();
    int height = readerUiDpi() / 4;
    bar.top = bar.bottom;
    if (_wm->getScreen()->isTouchSupported())
        bar.top -= height;
    _pressedCommand = 0;
    setRect(bar);
}

void CRSelNavigationDialog::draw()
{
    LVDrawBuf &buf = *_wm->getScreen()->getCanvas();
    LVDrawStateSaver state(buf);
    buf.FillRect(_rect, 0xFFFFFF);
    buf.SetTextColor(0x000000);
    LVFontRef font = fontMan->GetFont(_rect.height() * 2 / 5, 600, false,
                                     css_ff_sans_serif, lString8("DejaVu Sans"));
    const lString32 labels[] = { lString32(U"◀"), lString32(_("Done")), lString32(U"▶") };
    for (int i = 0; i < 3; ++i) {
        lvRect button = searchButtonRect(_rect, i);
        button.shrinkBy(lvRect(4, 4, 4, 4));
        buf.DrawFrame(button, 0x000000, 2);
        if (!font.isNull()) {
            const lString32 &label = labels[i];
            int width = font->getTextWidth(label.c_str(), label.length());
            font->DrawTextString(&buf, button.left + (button.width() - width) / 2,
                                 button.top + (button.height() - font->getHeight()) / 2,
                                 label.c_str(), label.length(), U'?');
        }
    }
}

bool CRSelNavigationDialog::onTouch(int x, int y, CRGUITouchEventType type)
{
    CRGUITouchEvent *event = new CRGUITouchEvent(x, y, type);
    event->setTargetWindow(this);
    _wm->postEvent(event);
    return true;
}

bool CRSelNavigationDialog::onTouchEvent(int x, int y, CRGUITouchEventType type)
{
    int command = 0;
    for (int i = 0; i < 3; ++i) {
        if (searchButtonRect(_rect, i).isPointInside(lvPoint(x, y))) {
            command = searchCommands[i];
            break;
        }
    }
    if (type == CRTOUCH_DOWN)
        _pressedCommand = command;
    if (type != CRTOUCH_UP)
        return false;
    int pressedCommand = _pressedCommand;
    _pressedCommand = 0;
    return command && command == pressedCommand && onCommand(command, 0);
}
#endif

void CRSelNavigationDialog::moveBy( int delta )
{
#if CR_POCKETBOOK==1
    int pageIndex = -1;
    if ( delta==1 ) {
        // forward
        pageIndex = _mainwin->findPagesText(_pattern, 1, 1);
        if (pageIndex == -1)
            pageIndex = _mainwin->findPagesText(_pattern, -1, 1);
    } else if ( delta==-1 ) {
        // backward
        pageIndex = _mainwin->findPagesText(_pattern, 1, -1);
        if (pageIndex == -1)
            pageIndex = _mainwin->findPagesText(_pattern, -1, -1);
    }
#else
    if ( delta==1 ) {
        // forward
        if ( !_mainwin->findText(_pattern, 1, 1) )
            _mainwin->findText(_pattern, -1, 1);
    } else if ( delta==-1 ) {
        // backward
        if ( !_mainwin->findText(_pattern, 1, -1) )
            _mainwin->findText(_pattern, -1, -1);
    }
    ldomMarkedRangeList * ranges = _mainwin->getDocView()->getMarkedRanges();
    if ( ranges ) {
        if ( ranges->length()>0 ) {
            int pos = ranges->get(0)->start.y;
            _mainwin->getDocView()->SetPos(pos);
        }
    }
#endif
    setDirty();
    _mainwin->setDirty();
}

CRSelNavigationDialog::CRSelNavigationDialog(  CRGUIWindowManager * wm, CRViewDialog * mainwin, lString32 pattern )
: BackgroundFitWindow(  wm, mainwin ), _mainwin(mainwin), _pattern(pattern)
{
#if CR_POCKETBOOK == 1
    setFullscreen(false);
    reconfigure(0);
#else
    _rect = _wm->getScreen()->getRect();
    _rect.top = _rect.bottom; // null height
#endif
    setAccelerators( _wm->getAccTables().get("dialog") );
    moveBy(0);
}


/// returns true if command is processed
bool CRSelNavigationDialog::onCommand( int command, int params )
{
    switch ( command ) {
    case MCMD_OK:
    case MCMD_CANCEL:
        {
            _mainwin->getDocView()->clearSelection();
            _wm->closeWindow( this );
        }
        return true;
#if CR_POCKETBOOK == 1
    case DCMD_PAGEDOWN:
#endif
    case MCMD_SCROLL_FORWARD:
        {
            moveBy( 1 );
        }
        break;
#if CR_POCKETBOOK == 1
    case DCMD_PAGEUP:
#endif
    case MCMD_SCROLL_BACK:
        {
            moveBy( -1 );
        }
        break;
    }
    return true;
}

