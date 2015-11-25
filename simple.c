/*	File:		simple.c	Contains:	What's in the Scrap?????    			This question comes in to DTS every so often, so here's the answer.    			First a NOTE:  This only applies to the in-memory scrap!  This is NOT    			the ScrapBook format, that is covered in a seperate tech note entitled    			Scrapbook File Format.    			The scrap is simply a handle containing chunks of data.  The basic format    			is    			OSType        OSType of this scrap data    			long         Size of this scrap data    			{variable}     The actual data   				So, you can walk through the scrap by getting the scrap handle (with InfoScrap)    			and advancing through the records.  That's what the function    			ParseScrap()    			in this small sample does, please steal as needed.    			To see multiple formats in the Scrap, launch Simple and switch to the Finder.    			Do a GetInfo on a Finder icon, select the icon in the Info Window, and do a    			'Copy'.  Now switch back to Simple and you'll see that the Finder has     			put 7 different types in the Scrap.    			Some interesting notes:    			� You can add multiple formats to the current scrap (like the Finder does for icons)    			just by calling PutScrap repeatedly.  PutScrap appends whatever data you pass in    			to the existing scrap handle with a simple PtrToHand(...) call.    			This means, by the way, that there is NO checking for the existence of that    			type in the scrap already!    			If you do a     			PutScrap(textLen,'TEXT', textPtr);    			PutScrap(textLen,'TEXT', textPtr);    			You will now have two TEXT's in the scrap handle.    			Of course, only the first one is accessable with GetScrap, because the Scrap Manager will     			stop when it finds the first.    			This happens because the Scrap Manager _assumes_ that you have called ZeroScrap    			before you call PutScrap.    			� Odd data.  If the data you add to the scrap has an odd length (like, 13 text     			characters) then the Scrap Manager adds 1 to make the handle even.  You'll notice    			I'm checking for and correcting for that in ParseScrap.	Written by: C.K. Haun		Copyright:	Copyright � 1991-1999 by Apple Computer, Inc., All Rights Reserved.				You may incorporate this Apple sample source code into your program(s) without				restriction. This Apple sample source code has been provided "AS IS" and the				responsibility for its operation is yours. You are not permitted to redistribute				this Apple sample source code as "Apple sample source code" after having made				changes. If you're going to re-distribute the source, we require that you make				it clear in the source that the code was descended from Apple sample source				code, but that you've made changes.	Change History (most recent first):				8/10/1999	Karl Groethe	Updated for Metrowerks Codewarror Pro 2.1				*//* Does a scrap parse now, when ever the window is redrawn  */#include <fonts.h>#include <dialogs.h>#include <Desk.h>#include <diskinit.h>#include <resources.h>#include <toolutils.h>#include <Gestalt.h>#include <Balloons.h>#include <Scrap.h>#include <TextUtils.h>#include "Simple.h"/* prototypes */void ParseScrap(void);void DrawIndString(short resID, short index);void WriteNum(long theNum);WindowPtr FindMyWindow(void);void InitalizeApp(void);void DoDiskEvents(long dinfo);                              /* hi word is error code, lo word is drive number */void DrawMain(WindowPtr drawIt);Boolean DoSelected(long val);void SizeMain(WindowPtr theWindow, short how);void InitAEStuff(void);void DoHighLevel(EventRecord *AERecord);void DoDaCall(MenuHandle themenu, long theit);void DoDocumentClick(WindowPtr theWindow, EventRecord *theEvent);void ActivateMain(WindowPtr theWindow, Boolean on);pascal OSErr AEOpenHandler(const AppleEvent *messagein, AppleEvent *reply, unsigned long refIn);pascal OSErr AEOpenDocHandler(const AppleEvent *messagein, AppleEvent *reply, unsigned long refIn);pascal OSErr AEPrintHandler(const AppleEvent *messagein, AppleEvent *reply, unsigned long refIn);pascal OSErr AEQuitHandler(const AppleEvent *messagein, AppleEvent *reply, unsigned long refIn);void SampleHelpDialog(void);WindowPtr AddNewWindow(short theID);void NilProc(void);/* one external */extern void _DataInit();                                    /* this is the C initialization code *//* globals */Boolean gQuit, gInBackground;unsigned long gMySleep;ProcessSerialNumber gOurSN;short gHelpItem;#pragma segment Mainvoid main(){    EventRecord myEventRecord;    WindowPtr twindow;    short fHit;    windowCHandle tempWCH;        //UnloadSeg((Ptr)_DataInit);                              /* throw out setup code */    InitalizeApp();    //UnloadSeg((Ptr)InitalizeApp);                           /* get rid of my initialization code */    do {        WaitNextEvent(everyEvent, &myEventRecord, gMySleep, nil);                switch (myEventRecord.what) {            case nullEvent:                                /* no nul processing in this sample */                break;            case updateEvt:                /* always check to see if it's my window */                /* this may not seem necessary under 7.0, where it's unlikely or impossible for */                /* a DA to be in your layer, but there are others */                /* who can stick themselves into your window list, */                /* BalloonWriter comes quickly to mind */                if (((WindowPeek)myEventRecord.message)->windowKind == kMyDocumentWindow) {                    tempWCH = (windowCHandle)GetWRefCon((WindowPtr)myEventRecord.message);                    ((*tempWCH)->drawMe)((WindowPtr)myEventRecord.message);                }                break;            case mouseDown:                /* first see where the hit was */                fHit = FindWindow(myEventRecord.where, &twindow);                switch (fHit) {                    Rect limitRect;                    Str255 tempString;                    long back;                    case inDesk:                            /* if they hit in desk, then the process manager */                        break;                              /* will switch us out, we don't need to do anything */                    case inMenuBar:                        DoSelected(MenuSelect(myEventRecord.where));                        break;                                            case inSysWindow:                        /* pass to the system */                        SystemClick(&myEventRecord, twindow);                        break;                    case inContent:                        /* Handle content and control clicks here */                        if (((WindowPeek)myEventRecord.message)->windowKind == kMyDocumentWindow) { /* don't do this unless we have a window open, silly */                            windowCHandle clicker;                            clicker = (windowCHandle)GetWRefCon(twindow);                            /* jump to the content function stored for this window */                            HLock((Handle)clicker);         /* lock it down so things don't get stupid */                            ((*clicker)->clickMe)(twindow, &myEventRecord);                            HUnlock((Handle)clicker);       /* all done */                        }                        break;                    case inDrag:                        DragWindow(twindow, myEventRecord.where, &qd.screenBits.bounds);                        break;                    case inGrow:                        /* Call GrowWindow here if you have a grow box */                        SetPort(twindow);                        limitRect = qd.screenBits.bounds;                        limitRect.top = kMinHeight;                        GetWTitle(twindow, tempString);                        /* I'm not letting the user shrink the window so */                        /* small that the title is truncated */                        limitRect.left = StringWidth(tempString) + 120;                        back = GrowWindow(twindow, myEventRecord.where, &limitRect);                                                if (back) {                            if (((WindowPeek)myEventRecord.message)->windowKind == kMyDocumentWindow) {                                windowCHandle tempWCH = (windowCHandle)GetWRefCon(twindow);                                Rect sizeRect = ((WindowPtr)twindow)->portRect;                                InvalRect(&sizeRect);                                sizeRect.top = sizeRect.bottom - 16;                                sizeRect.left = sizeRect.right - 16;                                EraseRect(&sizeRect);                                InvalRect(&sizeRect);                                SizeWindow(twindow, back & 0xffff, back >> 16, true);                                ((*tempWCH)->sizeMe)(twindow, fHit);                            }                        }                        InvalRect(&twindow->portRect);                                                break;                    case inGoAway:                        /* Click in Close box */                        if (((WindowPeek)myEventRecord.message)->windowKind == kMyDocumentWindow) {                            if (TrackGoAway(twindow, myEventRecord.where))                                ((*(windowCHandle)((WindowPeek)twindow)->refCon)->closeMe)(twindow);                        }                        break;                    case inZoomIn:                    case inZoomOut:                        if (TrackBox(twindow, myEventRecord.where, fHit)) {                            if (((WindowPeek)myEventRecord.message)->windowKind == kMyDocumentWindow) {                                windowCHandle tempWCH = (windowCHandle)GetWRefCon(twindow);                                SetPort(twindow);                                                                ZoomWindow(twindow, fHit, true);                                InvalRect(&twindow->portRect);                                ((*tempWCH)->sizeMe)(twindow, fHit);                            }                        }                }            case mouseUp:                /* don't care */                break;                /* same action for key or auto key */            case keyDown:            case autoKey:                if (myEventRecord.modifiers & cmdKey)                    DoSelected(MenuKey(myEventRecord.message & charCodeMask));                break;            case keyUp:                /* don't care */                break;            case diskEvt:                /* I don't do anything special for disk events, this just passes them */                /* to a function that checks for an error on the mount */                DoDiskEvents(myEventRecord.message);                break;            case activateEvt:                if (((WindowPeek)myEventRecord.message)->windowKind == kMyDocumentWindow) {                    tempWCH = (windowCHandle)GetWRefCon((WindowPtr)myEventRecord.message);                    ((*tempWCH)->activateMe)((WindowPtr)myEventRecord.message, myEventRecord.modifiers & activeFlag);                }                break;            case networkEvt:                /* don't care */                break;            case driverEvt:                /* don't care */                break;            case app4Evt:                switch ((myEventRecord.message >> 24) & 0x0FF) {        /* high byte of message */                    case suspendResumeMessage:              /* suspend/resume is also an activate/deactivate */                        gInBackground = (myEventRecord.message & kResumeMask) == 0;                        if (!gInBackground) {							WindowPtr tempWP,myWindow;							GetPort(&tempWP);							myWindow = FindMyWindow();							if(myWindow){							SetPort(myWindow);							/* make sure the window gets redrawn when we come forward */							InvalRect(&myWindow->portRect);							}							SetPort(tempWP);                        }                        break;                }                break;            default:                break;                /* This dispatches high level events (AppleEvents, for example) */                /* to our dispatch routine. This is NEW in the event loop for */                /* System 7 */            case kHighLevelEvent:                DoHighLevel(&myEventRecord);                break;                        }    }            while (gQuit != true);    }/* DoDaCall opens the requested DA. It's here as a seperate routine if you'd *//* like to perform some action or just know when a DA is opened in your *//* layer. Can be handy to track memory problems when a DA is opened *//* with an Option-open */void DoDaCall(MenuHandle themenu, long theit){    long qq;   	Str255 DAname;    GetMenuItemText(themenu, theit, DAname);    qq = OpenDeskAcc(DAname);}/* end DoDaCall *//* DoDiskEvents just checks the error code from the disk mount, *//* and puts up the 'Format' dialog (through DIBadMount) if need be *//* You can do much more here if you care about what disks are *//* in the drive */void DoDiskEvents(long dinfo)                               /* hi word is error code, lo word is drive number */{    short hival, loval, tommy;    Point fredpoint =  {        40, 40    };    hival = HiWord(dinfo);    loval = LoWord(dinfo);    if (hival != noErr)                                     /* something happened */ {        tommy = DIBadMount(fredpoint, dinfo);    }}/* draws my window. Pretty simple */void DrawMain(WindowPtr drawIt){    RgnHandle tempRgn;    Rect scratchRect;    BeginUpdate(drawIt);    SetPort(drawIt);	/* clear what's there */	EraseRgn(drawIt->visRgn);    scratchRect = drawIt->portRect;    scratchRect.top = scratchRect.bottom - 15;    scratchRect.left = scratchRect.right - 15;    tempRgn = NewRgn();    GetClip(tempRgn);    ClipRect(&scratchRect);    DrawGrowIcon(drawIt);    SetClip(tempRgn);    DisposeRgn(tempRgn);    /* draw some text */    ParseScrap();        EndUpdate(drawIt);}/* my menu action taker. It returns a Boolean which I usually ignore, but it *//* mught be handy someday *//* I usually use it in an application to determine if a keystroke was accepted *//* by a menu or whether it should be passed along to any other key acceptors */Boolean DoSelected(long val){    short loval, hival;    Boolean returnVal = false;    loval = LoWord(val);    hival = HiWord(val);        switch (hival) {                                        /* switch off the menu number selected */        case kAppleMenu:                                    /* Apple menu */            if (loval != 1) {                               /* if this was not About, it's a DA */                DoDaCall(GetMenuHandle(kAppleMenu), loval);            } else {                Alert(kAboutBox, nil);                      /* do about box */            }            returnVal = true;            break;        case kFileMenu:                                     /* File menu */            switch (loval) {                case kQuitItem:                    gQuit = true;                           /* only item */                    returnVal = true;                    break;                default:                    break;            }            break;        case kEditMenu:            /* edit menu junk */            /* don't care */            switch (loval) {            default:                break;            }            break;        case kToolsMenu:            /* add all your test stuff here */            switch (loval) {            default:                break;            }            break;        case kHMHelpMenuID:                                 /* Defined in Balloons.h */            /* I only care about this item. If anything else is returned here, I don't know what */            /* it is, so I leave it alone. Remember, the Help Manager chapter says that */            /* Apple reserves the right to add and change things in the Help menu */            if (loval == gHelpItem)                SampleHelpDialog();            break;                }    HiliteMenu(0);    return(returnVal);}void DoDocumentClick(WindowPtr theWindow, EventRecord *theEvent){    #pragma unused(theWindow,theEvent)}/* InitAEStuff installs my appleevent handlers */void InitAEStuff(void){    AEinstalls HandlersToInstall[] =  {        {            kCoreEventClass, kAEOpenApplication, AEOpenHandler        },  {            kCoreEventClass, kAEOpenDocuments, AEOpenDocHandler        },  {            kCoreEventClass, kAEQuitApplication, AEQuitHandler        },  {            kCoreEventClass, kAEPrintDocuments, AEPrintHandler        },         /* The above are the four required AppleEvents. */            };        OSErr aevtErr = noErr;    long aLong = 0;    Boolean gHasAppleEvents = false;    /* Check this machine for AppleEvents. If they are not here (ie not 7.0)    * then we exit */    gHasAppleEvents = (Gestalt(gestaltAppleEventsAttr, &aLong) == noErr);    /* The following series of calls installs all our AppleEvent Handlers.    * These handlers are added to the application event handler list that     * the AppleEvent manager maintains. So, whenever an AppleEvent happens    * and we call AEProcessEvent, the AppleEvent manager will check our    * list of handlers and dispatch to it if there is one.    */    if (gHasAppleEvents) {        register qq;        for (qq = 0; qq < ((sizeof(HandlersToInstall) / sizeof(AEinstalls))); qq++) {            aevtErr = AEInstallEventHandler(HandlersToInstall[qq].theClass, HandlersToInstall[qq].theEvent,                                            NewAEEventHandlerProc(HandlersToInstall[qq].theProc), 0, false);            if (aevtErr) {                ExitToShell();                              /* just fail, baby */            }        }    } else {        ExitToShell();    }}/* end InitAEStuff *//* I'm not doing error handling in this sample for clarities sake, you should. Hah, *//* easy for me to say, huh? */void DoHighLevel(EventRecord *AERecord){    OSErr myErr;    myErr = AEProcessAppleEvent(AERecord);    }/* end DoHighLevel *//* This is the standard Open Application event. */pascal OSErr AEOpenHandler(const AppleEvent *messagein, AppleEvent *reply, unsigned long refIn){    WindowPtr myWindow;#pragma unused (messagein,reply,refIn)    /* we of course don't do anything here in this simple app */    /* except open our window */    myWindow = AddNewWindow(kDocWindowResID);        return(noErr);}/* end AEOpenHandler *//* Open Doc, opens our documents. Remember, this can happen at application start AND *//* anytime else. If your app is up and running and the user goes to the desktop, hilites one *//* of your files, and double-clicks or selects Open from the finder File menu this event *//* handler will get called. Which means you don't do any initialization of globals here, or *//* anything else except open then doc. *//* SO-- Do NOT assume that you are at app start time in this *//* routine, or bad things will surely happen to you. */pascal OSErr AEOpenDocHandler(const AppleEvent *messagein, AppleEvent *reply, unsigned long refIn){#pragma unused (messagein,refIn,reply)    /* we of course don't do anything here */    return(errAEEventNotHandled);                           /* we have no docs, so no odoc events should come to us */}pascal OSErr AEPrintHandler(const AppleEvent *messagein, AppleEvent *reply, unsigned long refIn){                                                           /* no printing handler in yet, so we'll ignore this */    /* the operation is functionally identical to the ODOC event, with the additon */    /* of calling your print routine. */#pragma unused (messagein,refIn,reply)    /* we of course don't do anything here */    return(errAEEventNotHandled);                           /* we have no docs, so no pdoc events should come to us */}/* Standard Quit event handler, to handle a Quit event from the Finder, for example. *//* ����� DO NOT CALL EXITTOSHELL HERE ����� or you will never have a happy life. *//* OK, it's a few months after I wrote that comment, and I've seen a lot of code *//* come through DTS that calls ExitToShell from quit handlers. Let me explain... *//* When an AppleEvent Handler is called (like this quit handler) you are ALMOST *//* 100% in your application world. A5 is right, you can call any toolbox function, *//* you can call your own routines, everything _seems_ like you are in complete *//* control. Well, almost but not quite. The routine has been dispatch to from the *//* AppleEvent Manager's space, so you _must_ return to that at some point! *//* Which is why you can't call ETS from here. When you call ExitToShell from an *//* AE Handler, the most likely thing that happens is the FInder quits, and your *//* application keeps running. Which ain't what you want, y'know? *//* so, DON'T CALL EXITTOSHELL FROM AN APPLEEVENT HANDLER!!!!!!!!!!!!!! */pascal OSErr AEQuitHandler(const AppleEvent *messagein, AppleEvent *reply, unsigned long refIn){#pragma unused (messagein,refIn,reply)    gQuit = true;    return(noErr);}/* This is my sample help dialog. Does not do anything, expand as you need */void SampleHelpDialog(void){    DialogPtr tdial = GetNewDialog(kSampHelp, nil, (WindowPtr)-1);    short itemhit = 0;    while (itemhit != 1) {        ModalDialog(nil, &itemhit);    }    DisposeDialog(tdial);}#pragma segment Initializevoid InitalizeApp(void){    Handle myMenu;    MenuHandle helpHandle, appleMenuHandle;    StringHandle helpString;    short count;    long vers;    MaxApplZone();    InitGraf((Ptr)&qd.thePort);    InitFonts();    InitWindows();    InitMenus();    TEInit();    InitDialogs(nil);    InitCursor();    /* Check system version */    Gestalt(gestaltSystemVersion, &vers);    vers = (vers >> 8) & 0xf;                               /* shift result over and mask out major version number */    if (vers < 7) {        StopAlert(kBadSystem, nil);        ExitToShell();    }    InitAEStuff();    /* set up my menu junk */    myMenu = GetNewMBar(kMBarID);    SetMenuBar(myMenu);    appleMenuHandle = GetMenuHandle(kAppleMenu);    AppendResMenu(appleMenuHandle, 'DRVR');        /* now install my Help menu item in the Help Manager's menu */    HMGetHelpMenuHandle(&helpHandle);                       /* Get the Hlpe menu handle */    count = CountMItems(helpHandle);                        /* How many items are there? */    helpString = GetString(kHelpString);                    /* get my help string */    DetachResource((Handle)helpString);                             /* detach it */    HNoPurge((Handle)helpString);    MoveHHi((Handle)helpString);    HLock((Handle)helpString);    InsertMenuItem(helpHandle, *helpString, count + 1);       /* insert my item in the Help menu */    gHelpItem = CountMItems(helpHandle);                    /* The number of the item */        DrawMenuBar();    GetCurrentProcess(&gOurSN);                             /* Get our process serial number for later use, if needed */    }#pragma segment MainWindowPtr AddNewWindow(short theID){    windowCHandle setControls;    WindowPtr tempWP;    short cnt = 0;    tempWP = GetNewWindow(theID, 0, (WindowPtr)-1);         /* get a new window */    ((WindowPeek)tempWP)->windowKind = kMyDocumentWindow;       /* mark it as my document window */    setControls = (windowCHandle)NewHandleClear(sizeof(windowControl));     /* add our control structure to it */    SetWRefCon(tempWP, (long)setControls);                  /* stop stuffing refCon directly <ckh 1.0.3> */    HLock((Handle)setControls);                             /* lock it down while we fill it*/        /* add pointers to our procedures for drawing, saving, and closing */    /* This way, all I need is one dispatch point for drawing, closing */    /* or whatever, I don't have to case off the window kind to go to the */    /* correct routine. Kinda like object-oriented programming, but I won't */    /* admit that. */    (*setControls)->drawMe = DrawMain;    (*setControls)->clickMe = DoDocumentClick;    (*setControls)->sizeMe = SizeMain;    (*setControls)->activateMe = ActivateMain;    (*setControls)->generalData = NewHandle(0);        return(tempWP);}void ActivateMain(WindowPtr theWindow, Boolean on){#pragma unused(on)WindowPtr tempWP;GetPort(&tempWP);SetPort(theWindow);/* make sure the window gets redrawn when it's activated or deactivated */InvalRect(&theWindow->portRect);SetPort(tempWP);    }void SizeMain(WindowPtr theWindow, short how){	#pragma unused(how)    WindowPtr tempWP;    GetPort(&tempWP);    InvalRect(&theWindow->portRect);    SetPort(tempWP);}void NilProc(void){    }/* walks the current scrap Handle and spits out *//* the contents */void ParseScrap(void){    OSType currentType;    long dataLength;    short vPos = 20;    Size lengthOfHandle;	/* get the basic scrap information */    PScrapStuff theScrap = InfoScrap();	/* get the scrap handle */    Handle scrapHandle = theScrap->scrapHandle;	/* make a pointer to it */    Ptr theScrapPtr;	long currentLength;    Str15 theOSTypeString;	/* OS type will always be 4 */    theOSTypeString[0] = 4;	/* don't do anything if the handle doesn't exist */    if (scrapHandle) {        HLock(scrapHandle);		theScrapPtr = *scrapHandle;        MoveTo(10, vPos);        DrawIndString(kGeneralStrings, 1);        /* walk through all the types in the scrap */        vPos += 15;		/* how big the current scrap handle is */        lengthOfHandle = GetHandleSize(scrapHandle);        if (lengthOfHandle) {            do {                MoveTo(10, vPos);                DrawIndString(kGeneralStrings, 2);                currentType = *((OSTypePtr)theScrapPtr);        /* get this type */				/* block move it so 68000 machines don't b*tch */				/* about odd addresses */				BlockMove((Ptr)&currentType,(Ptr)&theOSTypeString[1],4);				/* draw the type */                DrawString(theOSTypeString);                theScrapPtr += 4;                           /* move 4 bytes */                dataLength = *((long *)theScrapPtr);        /* length of this type */                DrawIndString(kGeneralStrings, 3);                /* write the lenght */                WriteNum(dataLength);				/* next verticle */                vPos += 15;                                theScrapPtr += 4;                           /* now pointing at the data */                theScrapPtr += dataLength;                  /* mvoe to the next type */				/* round up, since the handle will always be even */				currentLength =	theScrapPtr - *scrapHandle;				/* if odd, add 1 */				if(currentLength & 0x1)currentLength++;								/* are we at the end of the data??? */                if (currentLength >= lengthOfHandle) {                    break;                                  /* at the end of the handle, break out */                }            }                    while (true);        }        HUnlock(scrapHandle);            }}void DrawIndString(short resID, short index){    Str255 theString;    GetIndString(theString, resID, index);    DrawString(theString);    }void WriteNum(long theNum){    Str32 theNumAsString;    NumToString(theNum, theNumAsString);    DrawString(theNumAsString);    }WindowPtr FindMyWindow(void){WindowPtr tempWP = FrontWindow();while(tempWP){if(((WindowPeek)tempWP)->windowKind == kMyDocumentWindow)break;else	tempWP = (WindowPtr)((WindowPeek)tempWP)->nextWindow;}return(tempWP);}