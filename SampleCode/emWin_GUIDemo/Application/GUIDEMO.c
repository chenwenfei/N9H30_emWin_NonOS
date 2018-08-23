/*********************************************************************
*                 SEGGER Software GmbH                               *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2018  SEGGER Microcontroller GmbH                *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V5.46 - Graphical user interface for embedded applications **
All  Intellectual Property rights in the Software belongs to  SEGGER.
emWin is protected by  international copyright laws.  Knowledge of the
source code may not be used to write a similar product. This file may
only be used in accordance with the following terms:

The  software has  been licensed by SEGGER Software GmbH to Nuvoton Technology Corporation
at the address: No. 4, Creation Rd. III, Hsinchu Science Park, Taiwan
for the purposes  of  creating  libraries  for its 
Arm Cortex-M and  Arm9 32-bit microcontrollers, commercialized and distributed by Nuvoton Technology Corporation
under  the terms and conditions  of  an  End  User  
License  Agreement  supplied  with  the libraries.
Full source code is available at: www.segger.com

We appreciate your understanding and fairness.
----------------------------------------------------------------------
Licensing information
Licensor:                 SEGGER Software GmbH
Licensed to:              Nuvoton Technology Corporation, No. 4, Creation Rd. III, Hsinchu Science Park, 30077 Hsinchu City, Taiwan
Licensed SEGGER software: emWin
License number:           GUI-00735
License model:            emWin License Agreement, signed February 27, 2018
Licensed platform:        Cortex-M and ARM9 32-bit series microcontroller designed and manufactured by or for Nuvoton Technology Corporation
----------------------------------------------------------------------
Support and Update Agreement (SUA)
SUA period:               2018-03-26 - 2019-03-27
Contact to extend SUA:    sales@segger.com
----------------------------------------------------------------------
File        : GUIDEMO.c
Purpose     : Several GUIDEMO routines
----------------------------------------------------------------------
*/

#include "GUIDEMO.h"

//
// Recommended memory to run the sample with adequate performance
//
#define RECOMMENDED_MEMORY (1024L * 2200)

/*********************************************************************
*
*       Dialog resources
*
**********************************************************************
*/
#if GUI_WINSUPPORT

static const GUI_WIDGET_CREATE_INFO _aFrameWinControl[] = {
  { FRAMEWIN_CreateIndirect, "Control",    0,               0,  0,  CONTROL_SIZE_X, CONTROL_SIZE_Y, 0,          0 },
  { BUTTON_CreateIndirect,   "Halt",       GUI_ID_HALT,     2,  24, BUTTON_SIZE_X,  BUTTON_SIZE_Y,  0,          0 },
  { BUTTON_CreateIndirect,   "Next",       GUI_ID_NEXT,     36, 24, BUTTON_SIZE_X,  BUTTON_SIZE_Y,  0,          0 },
  { PROGBAR_CreateIndirect,  0,            GUI_ID_PROGBAR0, 2,  11,  PROGBAR_SIZE_X, PROGBAR_SIZE_Y, WM_CF_HIDE, 0 },
  { TEXT_CreateIndirect,     0,            GUI_ID_TEXT0,    2,  2,  TEXT_SIZE_X,    TEXT_SIZE_Y,    0,          0 }
};

static const GUI_WIDGET_CREATE_INFO _aFrameWinInfo[] = {
  { FRAMEWIN_CreateIndirect, "emWin Demo", 0,               0,  0,  0,              0,              0,          0 },
  { TEXT_CreateIndirect,     "",           GUI_ID_TEXT1,    3,  3,  0,              0,              0,          0 }
};

#endif

/*********************************************************************
*
*       Static variables
*
**********************************************************************
*/
static   GUIDEMO_CONFIG _GUIDemoConfig;
#if GUI_WINSUPPORT
  static WM_HWIN _hDialogControl;
  static WM_HWIN _hDialogInfo;
  static int     _iDemoMinor;
#endif

#if GUI_SUPPORT_MEMDEV
  static int     _Pressed;
#endif

static   void (* _pfDrawBk)(void);
static   int     _iDemo;
static   int     _HaltTime;
static   int     _HaltTimeStart;
static   int     _Halt;
static   int     _Next;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/
/*********************************************************************
*
*       _cbBk
*/
#if GUI_WINSUPPORT
static void _cbBk(WM_MESSAGE * pMsg) {
  WM_KEY_INFO * pInfo;

  switch (pMsg->MsgId) {
  case WM_PAINT:
    _pfDrawBk();
    break;
  case WM_SET_FOCUS:
    pMsg->Data.v = 0;
    break;
  case WM_KEY:
    pInfo = (WM_KEY_INFO *)pMsg->Data.p;
    if (pInfo->PressedCnt) {
      switch (pInfo->Key) {
      case 'n':
        _Next = 1;
        break;
      }
    }
    break;
  default:
    WM_DefaultProc(pMsg);
    break;
  }
}
#endif

/*********************************************************************
*
*       _cbEffect
*/
#if GUI_SUPPORT_MEMDEV
static int _cbEffect(int TimeRem, void * pVoid) {
  GUI_PID_STATE State;
  int           Pressed;

  GUI_USE_PARA(TimeRem);
  GUI_Exec();
  GUI_PID_GetState(&State);
  if (State.Pressed) {
    *((int *)pVoid) = 1;
    return 0;
  } else {
    Pressed = *((int *)pVoid);
    if ((State.Pressed == 0) && (Pressed == 1)) {
      *((int *)pVoid) = 0;
      _Next           = 1;
      return 1;
    }
    _Next = GUIDEMO_CheckCancel();
    return _Next;
  }
}
#endif

/*********************************************************************
*
*       _DrawWindowBk
*/
static void _DrawWindowBk(WM_HWIN hWin) {
  int xSize, ySize;

  xSize = WM_GetWindowSizeX(hWin);
  ySize = WM_GetWindowSizeY(hWin);
  GUI_DrawGradientV(1, 0, xSize - 2, ySize - 1, GUI_MAKE_COLOR(0xF3F3E6), GUI_MAKE_COLOR(0xDCCEC0));
  GUI_SetColor(GUI_MAKE_COLOR(0xA8A851));
  GUI_DrawRect(0, -1, xSize - 1, ySize - 1);
}

/*********************************************************************
*
*       _cbFrameWinControl
*/
#if GUI_WINSUPPORT
static void _cbFrameWinControl(WM_MESSAGE * pMsg) {
  WM_HWIN hItem;
  int     NCode;
  int     Id;

  switch (pMsg->MsgId) {
  case WM_KEY:
    WM_SendMessage(WM_HBKWIN, pMsg);
    break;
  case WM_INIT_DIALOG:
    FRAMEWIN_SetFont(pMsg->hWin, &GUI_Font8_1);
//    FRAMEWIN_SetTitleHeight(pMsg->hWin, 14);
    hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_PROGBAR0);
    PROGBAR_SetTextAlign(hItem, GUI_TA_HCENTER | GUI_TA_VCENTER);
    PROGBAR_SetFont(hItem, &GUI_FontD6x8);
    hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_HALT);
    BUTTON_SetFocussable(hItem, 0);
    hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_NEXT);
    BUTTON_SetFocussable(hItem, 0);
    hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_TEXT0);
    TEXT_SetText(hItem, "Intro");
    TEXT_SetFont(hItem, &GUI_Font8_ASCII);
    break;
  case WM_PAINT:
    _DrawWindowBk(pMsg->hWin);
    break;
  case WM_NOTIFY_PARENT:
    Id    = WM_GetId(pMsg->hWinSrc);
    NCode = pMsg->Data.v;
    switch (NCode) {
    case WM_NOTIFICATION_RELEASED:
      switch (Id) {
      case GUI_ID_HALT:
        if (_Halt) {
          _Halt          = 0;
          _HaltTime      = GUI_GetTime() - _HaltTimeStart;
          WM_MakeModal(0);
        } else {
          _Halt          = 1;
          _HaltTimeStart = GUI_GetTime() - _HaltTime;
          WM_MakeModal(pMsg->hWin);
        }
        break;
      case GUI_ID_NEXT:
        _Next = 1;           // Will be set to 0 by GUIDEMO_GetNextState()
        GUIDEMO_ClearHalt(); // Clear _Halt, so the next sample demonstrates immediately
        break;
      }
      break;
    }
    break;
  default:
    WM_DefaultProc(pMsg);
  }
}
#endif

/*********************************************************************
*
*       _cbFrameWinInfo
*/
#if GUI_WINSUPPORT
static void _cbFrameWinInfo(WM_MESSAGE * pMsg) {
  WM_HWIN hItem;
  int     xSize;

  switch (pMsg->MsgId) {
  case WM_INIT_DIALOG:
    hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_TEXT1);
    TEXT_SetWrapMode(hItem, GUI_WRAPMODE_WORD);
    TEXT_SetTextColor(hItem, GUI_MAKE_COLOR(0x545428));
    break;
  case WM_KEY:
    WM_SendMessage(WM_HBKWIN, pMsg);
    break;
  case WM_CREATE:
    xSize = LCD_GetXSize();
    WM_SetWindowPos(pMsg->hWin, xSize / 2, 0, xSize / 2, INFO_SIZE_Y);
    break;
  case WM_PAINT:
    _DrawWindowBk(pMsg->hWin);
    break;
  default:
    WM_DefaultProc(pMsg);
  }
}
#endif

/*********************************************************************
*
*       _DrawBk
*/
#if GUIDEMO_USE_AUTO_BK
static void _DrawBk(void) {
  int xSize;
  int ySize;

  xSize = LCD_GetXSize();
  ySize = LCD_GetYSize();
  GUI_DrawGradientV(0, 0, xSize, ySize, BK_COLOR_0, BK_COLOR_1);
  GUI_DrawBitmap(&bmSeggerLogo70x35, LOGO_DIST_BORDER, LOGO_DIST_BORDER);
    GUI_DrawBitmap(&bmnuvoton_logo_70x35, LOGO_DIST_BORDER, LOGO_DIST_BORDER+bmnuvoton_logo_70x35.YSize);
}
#endif

/*********************************************************************
*
*       _DrawBkCircle
*/
#if (GUIDEMO_USE_AUTO_BK && GUI_SUPPORT_MEMDEV)
static void _DrawBkCircle(void) {
  static GUI_MEMDEV_Handle   hMemStretch;
  GUI_MEMDEV_Handle          hMemGradient;
  GUI_MEMDEV_Handle          hMemCircle;
  GUI_MEMDEV_Handle          hMemOld;
  int                        CircleWidth;
  int                        xSize;
  int                        ySize;
  int                        i;
  U32                      * pData;

  xSize  = LCD_GetXSize();
  ySize  = LCD_GetYSize();
  if (hMemStretch == 0) {
    CircleWidth  = (CIRCLE_RADIUS << 1) + 1;
    hMemCircle   = GUI_MEMDEV_CreateFixed32(0, 0, CircleWidth, CircleWidth);
    hMemStretch  = GUI_MEMDEV_CreateEx     (0, 0, xSize,       ySize,         GUI_MEMDEV_NOTRANS);
    hMemGradient = GUI_MEMDEV_CreateFixed32(0, 0, 1,           CIRCLE_RADIUS);
    //
    // Initialize background
    //
    hMemOld = GUI_MEMDEV_Select(hMemCircle);
    GUI_SetBkColor(BK_COLOR_1);
    GUI_Clear();
    //
    // Create Gradient
    //
    GUI_MEMDEV_Select(hMemGradient);
    GUI_DrawGradientV(0, 0, 0, CIRCLE_RADIUS, BK_COLOR_0, BK_COLOR_1);
    //
    // Get color and draw circles
    //
    pData = (U32 *)GUI_MEMDEV_GetDataPtr(hMemGradient);
    GUI_MEMDEV_Select(hMemCircle);
    for (i = 0; i < CIRCLE_RADIUS; i++, pData++) {
      GUI_SetColor(*pData);
      GUI_DrawCircle(CIRCLE_RADIUS, CIRCLE_RADIUS, i);
    }
    //
    // Stretch and display
    //
    GUI_MEMDEV_Select(hMemStretch);
    GUI_MEMDEV_DrawPerspectiveX(hMemCircle, 0, 0, ySize, ySize, xSize, 0);
    //
    // Putting logo into memory device avoids flickering effect
    //
    GUI_DrawBitmap(&bmSeggerLogo70x35, LOGO_DIST_BORDER, LOGO_DIST_BORDER);
        GUI_DrawBitmap(&bmnuvoton_logo_70x35, LOGO_DIST_BORDER, LOGO_DIST_BORDER+bmnuvoton_logo_70x35.YSize);
    //
    // Delete helper devices
    //
    GUI_MEMDEV_Delete(hMemCircle);
    GUI_MEMDEV_Delete(hMemGradient);
    GUI_MEMDEV_Select(hMemOld);
  }
  GUI_MEMDEV_Write(hMemStretch);
}
#endif

/*********************************************************************
*
*       _DrawBkSimple
*/
volatile U8 g_u8GUIItem;
static void _DrawBkSimple(void) {
  GUI_SetBkColor(BK_COLOR_1);
  GUI_Clear();
  GUI_DrawBitmap(&bmSeggerLogo70x35, LOGO_DIST_BORDER, LOGO_DIST_BORDER);
    if (g_u8GUIItem == 1)
        GUI_DrawBitmap(&bmnuvoton_logo_70x35, LOGO_DIST_BORDER+bmnuvoton_logo_70x35.XSize, LOGO_DIST_BORDER);
    else if (g_u8GUIItem == 2)
        GUI_DrawBitmap(&bmnuvoton_logo_70x35, (800 - bmnuvoton_logo_70x35.XSize), LOGO_DIST_BORDER);
    else
        GUI_DrawBitmap(&bmnuvoton_logo_70x35, LOGO_DIST_BORDER, LOGO_DIST_BORDER+bmnuvoton_logo_70x35.YSize);
}

/*********************************************************************
*
*       _FrameDrawSkinFlex
*/
#if GUI_WINSUPPORT
static int _FrameDrawSkinFlex(const WIDGET_ITEM_DRAW_INFO * pDrawItemInfo) {
  switch (pDrawItemInfo->Cmd) {
  case WIDGET_ITEM_CREATE:
    FRAMEWIN_SetTextAlign(pDrawItemInfo->hWin, GUI_TA_HCENTER | GUI_TA_VCENTER);
    FRAMEWIN_SetTextColor(pDrawItemInfo->hWin, GUI_BLACK);
    break;
  default:
    return FRAMEWIN_DrawSkinFlex(pDrawItemInfo);
  }
  return 0;
}
#endif

/*********************************************************************
*
*       _HideProgress
*/
#if GUI_WINSUPPORT
static void _HideProgress(void) {
  PROGBAR_Handle hProg;

  hProg = WM_GetDialogItem(_hDialogControl, GUI_ID_PROGBAR0);
  WM_HideWindow(hProg);
}
#endif

/*********************************************************************
*
*       _IntroduceDemo
*
*  Function description
*    Shows the GUIDEMO introduction screen which display the title of
*    the sample and a short description.
*/
static void _IntroduceDemo(const char * pTitle, const char * pDescription) {
  int FontDistY;
  int TimeWait;
  int xCenter;
  int yCenter;
  int xSize;
  int ySize;
  int i;

  xSize   = LCD_GetXSize();
  ySize   = LCD_GetYSize();
  xCenter = xSize / 2;
  yCenter = ySize / 2;
  #if GUIDEMO_SUPPORT_CURSOR
    GUIDEMO_ShowCursor();
  #endif
  #if GUI_WINSUPPORT
    WM_HideWindow(_hDialogInfo);
    WM_ShowWindow(_hDialogControl);
  #endif
  GUI_Exec();      
  GUIDEMO_DrawBk();
  GUI_SetColor(GUI_WHITE);
  //
  // Title
  //
  GUI_SetTextMode(GUI_TM_TRANS);
  GUI_SetFont(&GUI_FontRounded22);
  GUI_DispStringHCenterAt(pTitle, xCenter, 60);
  //
  // Description
  //
  GUI_SetFont(&GUI_FontSouvenir18);
  FontDistY = GUI_GetFontDistY();
  GUI_DispStringHCenterAt(pDescription, xCenter, yCenter - FontDistY + 10);
  //
  // Determine time to wait
  //
  i = 0;
  while (pDescription[i]) {
    i++;
  }
  TimeWait = i * CHAR_READING_TIME;
  GUIDEMO_Wait(TimeWait);
}

/*********************************************************************
*
*       _UpdateControlText
*/
#if GUI_WINSUPPORT
static void _UpdateControlText(void) {
  TEXT_Handle hText;
  char        acText[20] = { 0 };

  hText = WM_GetDialogItem(_hDialogControl, GUI_ID_TEXT0);
  GUIDEMO_AddStringToString(acText, "Demo ");
  GUIDEMO_AddIntToString   (acText, _iDemo + 1);
  GUIDEMO_AddStringToString(acText, ".");
  GUIDEMO_AddIntToString   (acText, _iDemoMinor);
  GUIDEMO_AddStringToString(acText, "/");
  GUIDEMO_AddIntToString   (acText, _GUIDemoConfig.NumDemos - 1);
  TEXT_SetText             (hText,  acText);
}
#endif
#if 0
/*********************************************************************
*
*       _DrawSkin_BUTTON
*/
static int _DrawSkin_BUTTON(const WIDGET_ITEM_DRAW_INFO * pDrawItemInfo) {
  const GUI_BITMAP * pBm;
  switch (pDrawItemInfo->Cmd) {
  case WIDGET_ITEM_DRAW_TEXT:
    GUI_SetColor(GUI_BLACK);
    switch(WM_GetId(pDrawItemInfo->hWin)) {
    case GUI_ID_HALT:
      if (BUTTON_IsPressed(pDrawItemInfo->hWin)) {
        GUI_SetColor(GUI_MAKE_COLOR(0xFFFFFF));
      } else {
        GUI_SetColor(GUI_MAKE_COLOR(0xA8A851));
      }
      if (_Halt) {
        pBm = &bmplay;
      } else {
        pBm = &bmstop;
      }
      GUI_DrawBitmap(pBm, ((pDrawItemInfo->x1 - pDrawItemInfo->x0) - pBm->XSize) / 2 + 1, ((pDrawItemInfo->y1 - pDrawItemInfo->y0) - pBm->YSize) / 2 + 1);
      break;
    case GUI_ID_NEXT:
      if (BUTTON_IsPressed(pDrawItemInfo->hWin)) {
        GUI_SetColor(GUI_MAKE_COLOR(0xFFFFFF));
      } else {
        GUI_SetColor(GUI_MAKE_COLOR(0xA8A851));
      }
      GUI_DrawBitmap(&bmforward, ((pDrawItemInfo->x1 - pDrawItemInfo->x0) - bmforward.XSize) / 2 + 1, ((pDrawItemInfo->y1 - pDrawItemInfo->y0) - bmforward.YSize) / 2 + 1);
      break;
    }
    break;
  case WIDGET_ITEM_DRAW_BACKGROUND:
    #if 0
      //
      // Windows8 style flat buttons
      //
      GUI_SetColor(GUI_MAKE_COLOR(0xA8A851));
      GUI_AA_DisableHiRes();
      if (BUTTON_IsPressed(pDrawItemInfo->hWin)) {
        GUI_AA_FillRoundedRect(pDrawItemInfo->x0 + 1, pDrawItemInfo->y0 + 1, pDrawItemInfo->x0 + BUTTON_SIZE_X - 2, pDrawItemInfo->y0 + BUTTON_SIZE_Y - 2, 4);
      } else {
        GUI_AA_DrawRoundedRect(pDrawItemInfo->x0 + 1, pDrawItemInfo->y0 + 1, pDrawItemInfo->x0 + BUTTON_SIZE_X - 2, pDrawItemInfo->y0 + BUTTON_SIZE_Y - 2, 4);
      }
    #else
      //
      // Glassy buttons
      //
      if (BUTTON_IsPressed(pDrawItemInfo->hWin)) {
        GUI_DrawBitmap(&bmbutton_1, pDrawItemInfo->x0, pDrawItemInfo->y0);
      } else {
        GUI_DrawBitmap(&bmbutton_0, pDrawItemInfo->x0, pDrawItemInfo->y0);
      }
    #endif
    break;
  default:
    //
    // Use the default skinning routine for processing all other commands
    //
    return BUTTON_DrawSkinFlex(pDrawItemInfo);
  }
  return 0;
}

/*********************************************************************
*
*       _DrawSkin_PROGBAR
*/
static int _DrawSkin_PROGBAR(const WIDGET_ITEM_DRAW_INFO * pDrawItemInfo) {
  int v, i, Min, Max, Pos;

  switch (pDrawItemInfo->Cmd) {
  case WIDGET_ITEM_DRAW_FRAME:
    GUI_SetColor(GUI_MAKE_COLOR(0xA8A851));
    GUI_DrawRect(pDrawItemInfo->x0, pDrawItemInfo->y0, pDrawItemInfo->x1, pDrawItemInfo->y1);
    break;
  case WIDGET_ITEM_DRAW_BACKGROUND:
    v = PROGBAR_GetValue(pDrawItemInfo->hWin);
    PROGBAR_GetMinMax(pDrawItemInfo->hWin, &Min, &Max);
    Pos = ((v * bmgrad_32x16.XSize * 4) / (Max - Min + 1)) % bmgrad_32x16.XSize;
    for (i = -1; i < 3; i++) {
      GUI_DrawBitmap(&bmgrad_32x16, bmgrad_32x16.XSize * i + Pos + 1, 1);
    }
    break;
  case WIDGET_ITEM_DRAW_TEXT:
    //
    // Do not show any text...
    //
    break;
  default:
    //
    // Use the default skinning routine for processing all other commands
    //
    return PROGBAR_DrawSkinFlex(pDrawItemInfo);
  }
  return 0;
}

/*********************************************************************
*
*       _DrawSkin_FRAMEWIN
*/
static int _DrawSkin_FRAMEWIN(const WIDGET_ITEM_DRAW_INFO * pDrawItemInfo) {
  GUI_RECT Rect;
  char acBuffer[20];

  switch (pDrawItemInfo->Cmd) {
  case WIDGET_ITEM_GET_BORDERSIZE_B:
  case WIDGET_ITEM_GET_BORDERSIZE_L:
  case WIDGET_ITEM_GET_BORDERSIZE_R:
  case WIDGET_ITEM_GET_BORDERSIZE_T:
    return 0;
  case WIDGET_ITEM_DRAW_BACKGROUND:
    GUI_SetBkColor(GUI_MAKE_COLOR(0xA8A851));
    GUI_Clear();
    break;
  case WIDGET_ITEM_DRAW_TEXT:
    Rect.x0 = pDrawItemInfo->x0;
    Rect.x1 = pDrawItemInfo->x1;
    Rect.y0 = pDrawItemInfo->y0;
    Rect.y1 = pDrawItemInfo->y1;
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_ASCII);
    GUI_SetTextMode(GUI_TM_TRANS);
    FRAMEWIN_GetText(pDrawItemInfo->hWin, acBuffer, sizeof(acBuffer));
    GUI_DispStringInRect(acBuffer, &Rect, GUI_TA_HCENTER | GUI_TA_VCENTER);
    break;
  case WIDGET_ITEM_DRAW_SEP:
    GUI_SetColor(GUI_MAKE_COLOR(0xC0C0C0));
    GUI_FillRect(pDrawItemInfo->x0, pDrawItemInfo->y0, pDrawItemInfo->x1,pDrawItemInfo->y1);
    break;
  default:
    //
    // Use the default skinning routine for processing all other commands
    //
    return _FrameDrawSkinFlex(pDrawItemInfo);
  }
  return 0;
}
#endif
/*********************************************************************
*
*       _Main
*/
static void _Main(void) {
  #if GUI_WINSUPPORT
    int xSize;
    int ySize;
//    WM_HWIN hItem;

    WM_SelectWindow(WM_HBKWIN);
  #endif
  GUI_Clear();
  #if GUIDEMO_SUPPORT_CURSOR
    GUIDEMO_ShowCursor();
  #endif
  //
  // Create and configure Control and Information window
  //
  #if GUI_WINSUPPORT
    xSize           = LCD_GetXSize();
    ySize           = LCD_GetYSize();
    //
    // Set skinning functions for control- and info window
    //
//    FRAMEWIN_SetDefaultSkin(_DrawSkin_FRAMEWIN);
//    BUTTON_SetDefaultSkin(_DrawSkin_BUTTON);
//    PROGBAR_SetDefaultSkin(_DrawSkin_PROGBAR);
    //
    // Create control- and info window
    //
    _hDialogControl = GUI_CreateDialogBox(_aFrameWinControl, GUI_COUNTOF(_aFrameWinControl), _cbFrameWinControl, WM_HBKWIN, xSize - CONTROL_SIZE_X, ySize - CONTROL_SIZE_Y);
    _hDialogInfo    = GUI_CreateDialogBox(_aFrameWinInfo,    GUI_COUNTOF(_aFrameWinInfo),    _cbFrameWinInfo,    WM_HBKWIN, xSize / 2 - 1,          0                     );
    WM_HideWindow(_hDialogInfo);
    //
    // Reset skinning functions to (demo)defaults
    //
//    FRAMEWIN_SetDefaultSkin(_FrameDrawSkinFlex);
//    BUTTON_SetDefaultSkin(BUTTON_SKIN_FLEX);
//    PROGBAR_SetDefaultSkin(PROGBAR_SKIN_FLEX);
    //
    // Hide ugly text, may be (re)activated in a later version
    //
//    hItem = WM_GetDialogItem(_hDialogControl, GUI_ID_TEXT0);
//    WM_HideWindow(hItem);
    //
    // Show Intro
    //
    WM_InvalidateWindow(WM_HBKWIN);
    WM_DisableMemdev(WM_HBKWIN);
    GUI_Exec();
    WM_EnableMemdev(WM_HBKWIN);
  #endif
  GUIDEMO_Intro();
  //
  // Run the demos
  //
  for (_iDemo = 0; _GUIDemoConfig.apFunc[_iDemo]; _iDemo++) {
    GUIDEMO_ClearHalt();
    #if GUI_WINSUPPORT
      _UpdateControlText();
    #endif
    (*_GUIDemoConfig.apFunc[_iDemo])();
    #if GUI_WINSUPPORT
      _iDemoMinor = 0;
    #endif
    #if GUI_SUPPORT_MEMDEV
      _Pressed = 0;
    #endif
  }
  _iDemo = 0;
  //
  // Cleanup
  //
  #if GUI_WINSUPPORT
    WM_DeleteWindow(_hDialogControl);
    WM_DeleteWindow(_hDialogInfo);
  #endif
}

/*********************************************************************
*
*       _ShowProgress
*/
#if GUI_WINSUPPORT
static void _ShowProgress(void) {
  PROGBAR_Handle hProg;

  hProg = WM_GetDialogItem(_hDialogControl, GUI_ID_PROGBAR0);
  WM_ShowWindow(hProg);
}
#endif

/*********************************************************************
*
*       Public functions (Cursor only)
*
**********************************************************************
*/
/*********************************************************************
*
*       GUIDEMO_HideCursor
*
*  Function description
*    Hides the cursor according to the preprocessor settings and config flags.
*/
#if GUIDEMO_SUPPORT_CURSOR
  void GUIDEMO_HideCursor(void) {
    if (GUIDEMO_GetConfFlag(GUIDEMO_CF_SUPPORT_TOUCH)) {
      GUI_CURSOR_Hide();
    }
  }
#endif

/*********************************************************************
*
*       GUIDEMO_ShowCursor
*
*  Function description
*    Shows the cursor according to the preprocessor settings and config flags.
*/
#if GUIDEMO_SUPPORT_CURSOR
  void GUIDEMO_ShowCursor(void) {
    if (GUIDEMO_GetConfFlag(GUIDEMO_CF_SUPPORT_TOUCH)) {
      GUI_CURSOR_Show();
    }
  }
#endif

/*********************************************************************
*
*       Public functions (WM only)
*
**********************************************************************
*/
/*********************************************************************
*
*       GUIDEMO_Delay
*
*  Function description
*    This function has to be called if the current step of the sample
*    is the last one and consists of a single frame.
*/
#if GUI_WINSUPPORT
void GUIDEMO_Delay(int TimeDelay) {
  PROGBAR_Handle hProg;
  int            NextState;
  U32            TimeStart;
  U32            TimeDiff;

  hProg = WM_GetDialogItem(_hDialogControl, GUI_ID_PROGBAR0);
  if (TimeDelay > SHOW_PROGBAR_AT) {
    _ShowProgress();
  }
  PROGBAR_SetValue(hProg, 0);
  PROGBAR_SetMinMax(hProg, 0, TimeDelay);
  TimeStart     = GUI_GetTime();
  do {
    TimeDiff = GUIDEMO_GetTime() - TimeStart;
    if (TimeDelay > SHOW_PROGBAR_AT) {
      PROGBAR_SetValue(hProg, TimeDiff);
      WM_InvalidateWindow(hProg);
    }
    GUI_Delay(5);
    NextState = GUIDEMO_CheckCancel();
  } while (TimeDiff < (U32)TimeDelay && !NextState);
  if (TimeDelay > SHOW_PROGBAR_AT) {
    _HideProgress();
  }
}
#endif

/*********************************************************************
*
*       GUIDEMO_NotifyStartNext
*
*   Use this function if the next step of the current sample will be
*   shown immediately
*/
#if GUI_WINSUPPORT
void GUIDEMO_NotifyStartNext(void) {
  _iDemoMinor++;
  GUIDEMO_ClearHalt();
  _UpdateControlText();
}
#endif

/*********************************************************************
*
*       GUIDEMO_SetInfoText
*/
#if GUI_WINSUPPORT
void GUIDEMO_SetInfoText(const char * pInfo) {
  TEXT_Handle hText;

  if (WM_IsVisible(_hDialogInfo)) {
    hText = WM_GetDialogItem(_hDialogInfo, GUI_ID_TEXT1);
    TEXT_SetText(hText, pInfo);
  }
}
#endif

/*********************************************************************
*
*       Public functions (general)
*
**********************************************************************
*/
/*********************************************************************
*
*       GUIDEMO_AddIntToString
*/
void GUIDEMO_AddIntToString(char * pText, U32 Number) {
  int TextLen;
  int LenNum;
  U32 i;

  TextLen = 0;
  while (*(pText + TextLen)) {
    TextLen++;
  }
  i       = 1;
  LenNum  = 1;
  while ((Number / i) >= 10) {
    i *= 10;
    LenNum++;
  }
  *(pText + TextLen + LenNum) = '\0';
  while (LenNum) {
    *(pText + TextLen + LenNum - 1) = '0' + Number % 10;
    Number /= 10;
    LenNum--;
  }
}

/***************i******************************************************
*
*       GUIDEMO_AddStringToString
*/
void GUIDEMO_AddStringToString(char * pText, const char * pAdd) {
  int i;
  int j;

  i = 0;
  j = 0;
  while (*(pText + i)) {
    i++;
  }
  while (*(pAdd + j)) {
    *(pText + i) = *(pAdd + j);
    i++;
    j++;
  }
  *(pText + i) = '\0';
}

/*********************************************************************
*
*       GUIDEMO_CheckCancel
*/
int GUIDEMO_CheckCancel(void) {
  //
  // Do not return until the button is released
  //
  while (_Halt == 1) {
    GUI_Delay(10);
  }
  //
  // Check Next Button
  //
  if (_Next == 1) {
    _Next = 0;
    return 1;
  }
  return 0;
}

/*********************************************************************
*
*       GUIDEMO_CheckCancelDelay
*
*  Function description:
*    Checks for cancel input every 10 ms for the given amount of time.
*/
int GUIDEMO_CheckCancelDelay(int Delay) {
  int TimeNow;
  int TimeEnd;

  TimeNow = GUI_GetTime();
  TimeEnd = TimeNow + Delay;
  do {
    GUI_Delay(10);
    TimeNow = GUI_GetTime();
    if (GUIDEMO_CheckCancel()) {
      GUIDEMO_NotifyStartNext();
      return 1;
    }
  } while (TimeNow < TimeEnd);
  return 0;
}

/*********************************************************************
*
*       GUIDEMO_ClearHalt
*
*   This function is called if the next button is pressed after
*   the demo was halted.
*/
void GUIDEMO_ClearHalt(void) {
  _Halt          = 0;
  _HaltTime      = 0;
  _HaltTimeStart = 0;
}

/*********************************************************************
*
*       GUIDEMO_ClearText
*
*/
void GUIDEMO_ClearText(char * pText) {
  *pText = 0;
}

/*********************************************************************
*
*       GUIDEMO_ConfigureDemo
*
*  Function description
*    Configure the GUIDEMO for the current sample. Show the introduction
*    And change the visibility of the cursor, info- and control-window.
*
*    The following defines may be used as flags:
*    - GUIDEMO_SHOW_CURSOR
*    - GUIDEMO_SHOW_INFO
*    - GUIDEMO_SHOW_CONTROL
*
*    If a flag is not set this means that the according feature is turned off.
*/
void GUIDEMO_ConfigureDemo(char * pTitle, char * pDescription, unsigned Flags) {
  if (pTitle && pDescription) {
    _IntroduceDemo(pTitle, pDescription);
  }
  #if GUIDEMO_SUPPORT_CURSOR
    if (Flags & GUIDEMO_SHOW_CURSOR) {
      GUIDEMO_ShowCursor();
    } else {
      GUIDEMO_HideCursor();
    }
  #endif
  #if GUI_WINSUPPORT
    if (Flags & GUIDEMO_SHOW_INFO) {
      WM_ShowWindow(_hDialogInfo);
    } else {
      WM_HideWindow(_hDialogInfo);
    }
    if (Flags & GUIDEMO_SHOW_CONTROL) {
      WM_ShowWindow(_hDialogControl);
    } else {
      WM_HideWindow(_hDialogControl);
    }
  #endif
}

/*********************************************************************
*
*       GUIDEMO_DispTitle
*/
void GUIDEMO_DispTitle(char * pTitle) {
  GUI_RECT RectTitle;
  int      StringLen;
  int      xSize;

  GUI_SetFont(&GUI_FontRounded22);
  StringLen = GUI_GetStringDistX(pTitle);
  xSize     = LCD_GetXSize();
  if ((xSize - StringLen) / 2 - 5 < bmSeggerLogo70x35.XSize + LOGO_DIST_BORDER)  {
    RectTitle.x0 = bmSeggerLogo70x35.XSize + LOGO_DIST_BORDER;
  } else {
    RectTitle.x0 = 0;
  }
  RectTitle.y0 = 0;
  RectTitle.x1 = xSize - 1;
  RectTitle.y1 = bmSeggerLogo70x35.YSize + 2 * LOGO_DIST_BORDER - 1;
  GUI_SetTextMode(GUI_TM_TRANS);
  GUI_DispStringInRect(pTitle, &RectTitle, GUI_TA_HCENTER | GUI_TA_VCENTER);
}
                  
/*********************************************************************
*
*       GUIDEMO_DispHint
*/
void GUIDEMO_DispHint(char * pHint) {
  GUI_RECT RectHint;
  int      StringLen;
  int      xSize;
  int      ySize;

  GUI_SetFont(&GUI_FontRounded16);
  StringLen = GUI_GetStringDistX(pHint);
  xSize     = LCD_GetXSize();
  ySize     = LCD_GetYSize();
  if ((xSize - StringLen) / 2 - 5 < CONTROL_SIZE_X)  {
    RectHint.x1 = xSize - 1 - CONTROL_SIZE_X;
  } else {
    RectHint.x1 = xSize - 1;
  }
  RectHint.x0 = 0;
  RectHint.y0 = ySize - 1 - CONTROL_SIZE_Y;
  RectHint.y1 = ySize - 1;
  GUI_SetTextMode(GUI_TM_TRANS);
  GUI_DispStringInRect(pHint, &RectHint, GUI_TA_HCENTER | GUI_TA_VCENTER);
}
                  
/*********************************************************************
*
*       GUIDEMO_DrawBk
*/
void GUIDEMO_DrawBk(void) {
  _pfDrawBk();
}

/*********************************************************************
*
*       GUIDEMO_GetConfFlag
*/
U16 GUIDEMO_GetConfFlag(U16 Flag) {
  return (_GUIDemoConfig.Flags & Flag) ? 1 : 0;
}

/*********************************************************************
*
*       GUIDEMO_GetTime
*/
int GUIDEMO_GetTime(void) {
  return _Halt ? _HaltTimeStart : GUI_GetTime() - _HaltTime;
}

/*********************************************************************
*
*       GUIDEMO_GetTitleSizeY
*/
int GUIDEMO_GetTitleSizeY(void) {
  return bmSeggerLogo70x35.YSize + 2 * LOGO_DIST_BORDER;
}

/*********************************************************************
*
*       GUIDEMO_Main
*/
void GUIDEMO_Main(void) {
  #if GUI_WINSUPPORT
    FRAMEWIN_SKINFLEX_PROPS Framewin_Props;
  #endif
  #if GUIDEMO_USE_AUTO_BK
    U32                     NumFreeBytes;
    int                     BitsPerPixel;
  #endif
  
  //
  // Check if recommended memory for the sample is available
  //
  if (GUI_ALLOC_GetNumFreeBytes() < RECOMMENDED_MEMORY) {
    GUI_ErrorOut("Not enough memory available."); 
    return;
  }
  #if GUI_SUPPORT_MEMDEV
    GUI_MEMDEV_SetAnimationCallback(_cbEffect, (void *)&_Pressed);
  #endif
  #if GUI_WINSUPPORT
    WM_SetCallback(WM_HBKWIN, _cbBk);
    BUTTON_SetReactOnLevel();
    FRAMEWIN_GetSkinFlexProps(&Framewin_Props, FRAMEWIN_SKINFLEX_PI_ACTIVE);
    Framewin_Props.Radius = 0;
    FRAMEWIN_SetSkinFlexProps(&Framewin_Props, FRAMEWIN_SKINFLEX_PI_ACTIVE);
    FRAMEWIN_GetSkinFlexProps(&Framewin_Props, FRAMEWIN_SKINFLEX_PI_INACTIVE);
    Framewin_Props.Radius = 0;
    FRAMEWIN_SetSkinFlexProps(&Framewin_Props, FRAMEWIN_SKINFLEX_PI_INACTIVE);
    FRAMEWIN_SetDefaultSkin  (_FrameDrawSkinFlex);
    PROGBAR_SetDefaultSkin   (PROGBAR_SKIN_FLEX);
    BUTTON_SetDefaultSkin    (BUTTON_SKIN_FLEX);
    SCROLLBAR_SetDefaultSkin (SCROLLBAR_SKIN_FLEX);
    SLIDER_SetDefaultSkin    (SLIDER_SKIN_FLEX);
    HEADER_SetDefaultSkin    (HEADER_SKIN_FLEX);
  #endif
  GUI_SetTextMode(GUI_TM_TRANS);
  GUIDEMO_Config(&_GUIDemoConfig);
  #if GUIDEMO_USE_VNC
    if (GUIDEMO_GetConfFlag(GUIDEMO_CF_USE_VNC)) {
      _GUIDemoConfig.pGUI_VNC_X_StartServer(0, 0);
    }
  #endif
  #if GUIDEMO_USE_AUTO_BK
    //
    // Determine if HW has enough memory to draw the gradient circle as background
    //
    BitsPerPixel = LCD_GetBitsPerPixel();
    if ((BitsPerPixel >= 16) && GUIDEMO_GetConfFlag(GUIDEMO_CF_USE_AUTO_BK)) {
      NumFreeBytes = GUI_ALLOC_GetNumFreeBytes();
      if (NumFreeBytes > NUMBYTES_NEEDED) {
        #if GUI_SUPPORT_MEMDEV
          _pfDrawBk = _DrawBkCircle;
        #else
          _pfDrawBk = _DrawBk;
        #endif
      } else {
        _pfDrawBk = _DrawBk;
      }
    } else
  #endif
    {
      _pfDrawBk = _DrawBkSimple;
    }
  while (1) {
    _Main();
  }
}

/*********************************************************************
*
*       GUIDEMO_MixColors
*/
GUI_COLOR GUIDEMO_MixColors(GUI_COLOR Color, GUI_COLOR Color0, U8 Intens) {
  U32 r, g, b, a;
  U8  Intens0;

#if (GUI_USE_ARGB)
  if (Color0 & 0xFF000000) {
#else
  if ((Color0 & 0xFF000000) != 0xFF000000) {
#endif
    //
    // Calculate Color separations for FgColor first
    //
    r = ( Color & 0x000000ff)        * Intens;
    g = ( Color & 0x0000ff00)        * Intens;
    b = ( Color & 0x00ff0000)        * Intens;
    a = ((Color & 0xff000000) >> 24) * Intens;
    //
    // Add Color separations for Color0
    //
    Intens0 = 255 - Intens;
    r += ( Color0 & 0x000000ff)        * Intens0;
    g += ( Color0 & 0x0000ff00)        * Intens0;
    b += ( Color0 & 0x00ff0000)        * Intens0;
    a += ((Color0 & 0xff000000) >> 24) * Intens0;
    r =  (r >> 8) & 0x000000ff;
    g =  (g >> 8) & 0x0000ff00;
    b =  (b >> 8) & 0x00ff0000;
    a =  (a << (24 - 8)) & 0xff000000;
    Color = r | g | b | a;
  }
  return Color;
}

/*********************************************************************
*
*       GUIDEMO_ShiftRight
*/
int GUIDEMO_ShiftRight(int Value, U8 NumBits) {
  int Shift;
  U8  i;

  Shift = 2;
  for (i = NumBits - 1; i > 0; i--) {
    Shift *= 2;
  }
  return Value / Shift;
}

/*********************************************************************
*
*       GUIDEMO_Wait
*
*   This function has to be called if the current step is a static
*   frame and another step will follow
*/
void GUIDEMO_Wait(int TimeWait) {
  GUIDEMO_Delay(TimeWait);
  GUIDEMO_NotifyStartNext();
}

/*************************** End of file ****************************/
