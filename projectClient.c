#include <PalmOS.h>
#include "projectClient.h"

///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	Global Variables
//
//	These variables need to be used in more than
//		function.
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

//These varibles hold the overridden IP and port
Char * IPAddr;
int PORT;
Boolean override;

//This variable holds the active check number
Char * CHECKNUM;

//This is a reference to the net library and the socket
UInt16 gNetRefNum;
NetSocketRef socket;


//This ptr keeps the number of new messages
Char *numMsgs;

//This array holds the dynamic msg list and size
Char ** gMsgArray = 0;
UInt16 gMsgNum;

//This pointer holds the username to reply to
Char *replyTo;


//This array holds the dynamic item list and size
Char ** gItemsArray = 0;
UInt16 gItemsNum;

//This array holds the dynamic order list and size
Char ** gOrdersArray = 0;
UInt16 gOrdersNum;

//This array holds the current items for a check
Char ** gCheckArray = 0;
UInt16 gCheckNum;

//This ptr keeps the price total of the current check
Char *checkTotal;















///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	Utility Functions
//
//	Helper functions to perform a variety of tasks
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

//Get a pointer to a object on the current form by its ID
static MemPtr GetObjectPtr(UInt16 objectID) {
	
	FormPtr form;
	form = FrmGetActiveForm();
	return(FrmGetObjectPtr(form, FrmGetObjectIndex(form, objectID)));
	
}


//Get nth argument in a command string
//Args are separted by a '~' character
static void GetArg(Char * buffer , int n , Char * command) {
	
	Char *start, *end, *ptr;
	int i;
	
	start = (Char *)MemPtrNew(1024);
	StrCopy(start , command);
	
	//Get start ptr
	for(i = 0 ; i < n ; i++) {
		ptr = StrChr(start,'~');
		StrCopy(start,ptr);
		TxtSetNextChar(start,0,' ');
	}
	
	//Get end ptr
	if(!(end = StrChr(start,'~'))) {
		StrCat(start,"\0");
		StrCopy(buffer,start);
		MemPtrFree(start);
		return;
	}
	
	*end = '\0';
	StrCopy(buffer,start);
	MemPtrFree(start);
	return;
	
}



//Flush the incoming buffer
static void FlushBuffer() {
	
	Err error = 0;
	UInt16 bytesRead;
	Char *dataP;
	
	dataP = MemPtrNew(1);
	do {
		bytesRead = NetLibReceive(gNetRefNum,
								  socket,
								  dataP,
								  1,
								  0,
								  NULL,
								  0,
								  SysTicksPerSecond()/2,
								  &error);
	}
	while(bytesRead > 0 && !error);

	MemPtrFree(dataP);
}















///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	SendLine()
//
//	Send a line of text to the server
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static Boolean SendLine(Char * text) {
	
	Err error = 0;
	UInt16 bytesToSend;
	UInt16 bytesSent;
	Char *dataP;
	
	bytesToSend = StrLen(text);
	dataP = text;
	
	while((bytesToSend > 0) && (!error)) {
		bytesSent = NetLibSend(gNetRefNum,
							   socket,
							   dataP,
							   bytesToSend,
							   0,
							   NULL,
							   0,
							   3*SysTicksPerSecond(),
							   &error);
							   
		if(bytesSent == 0) return false;
		
		if(!error) {				   
			dataP+= bytesSent;
			bytesToSend -= bytesSent;
		}
	}
	
	bytesSent = NetLibSend(gNetRefNum,
				socket,
				"\n",
				1,
				0,
				NULL,
				0,
				10*SysTicksPerSecond(),
				&error);
	if(bytesSent == 0) return false;
	if(error) return false;
	
	return true;
	
}










///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	GetLine()
//
//	Get a line of text from the server (line term-
//		inated with '\n'
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static Boolean GetLine(Char * buffer) {
	
	Err error = 0;
	UInt16 bytesRead;
	Char *dataP;
	
	dataP = buffer;
	do {
		bytesRead = NetLibReceive(gNetRefNum,
								  socket,
								  dataP,
								  1,
								  0,
								  NULL,
								  0,
								  5*SysTicksPerSecond(),
								  &error);
	
		if((bytesRead == 0) || error) {
			return false;
		}
		else {
			if(!TxtCharIsPrint(*dataP)) break;
			dataP+=bytesRead;
			*dataP = '\0';
		}
	}
	while(true);
	*dataP = '\0';
	
	return true;
		
}












///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	Login Function
//
//	Attempts to log the user into server and returns
//		result.
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static Boolean Login() {
	
	Char *command, *returnCode, *num;
	FieldType *user, *pass;
	Int16 result;
	
	Err error;
	NetIPAddr address;
	NetSocketAddrINType destAddr;
	
	
	//Open a socket
	socket = NetLibSocketOpen(gNetRefNum,             
                          	  netSocketAddrINET,     
                          	  netSocketTypeStream,   
                          	  netSocketProtoIPTCP,    
                          	  5*SysTicksPerSecond(),        
                          	  &error);
    if(error) {
		NetLibSocketClose(gNetRefNum,socket,10*SysTicksPerSecond(),&error);
    	return false;
    }
    
	if(override) address = NetLibAddrAToIN(gNetRefNum, IPAddr);  //Override IP
	else address = NetLibAddrAToIN(gNetRefNum, "67.171.77.22");  //Use default
	
	MemSet(&destAddr, sizeof(destAddr), 0);
	destAddr.family = netSocketAddrINET; 
	if(override) destAddr.port = (UInt16) PORT;
	else destAddr.port = 2600;
	destAddr.addr = address;
	error = 0;
	
	//Connect socket
	result = NetLibSocketConnect(gNetRefNum,
                       			 socket,              
                       			 (NetSocketAddrType*)&destAddr,
                       			 sizeof(destAddr),              
                       			 5*SysTicksPerSecond(),                            
                       			 &error);
    if(result == -1) {
    	NetLibSocketShutdown(gNetRefNum,socket,netSocketDirBoth,10*SysTicksPerSecond(),&error);
    	NetLibSocketClose(gNetRefNum,socket,10*SysTicksPerSecond(),&error);
    	return false;
    }               
	
	
	//Attempt to login w/ username and password
	user = GetObjectPtr(LoginUsernameField);
	pass = GetObjectPtr(LoginPasswordField);
	
	
	command = (Char *)MemPtrNew(64);
	StrCopy(command,"01~");
	StrCat(command,FldGetTextPtr(user));
	StrCat(command,"~");
	StrCat(command,FldGetTextPtr(pass));
	StrCat(command,"\0");
	
	result = SendLine(command);
	MemPtrFree(command);
	
	
	//Get return code from server
	returnCode = (Char *)MemPtrNew(64);
	num = (Char *)MemPtrNew(64);
	GetLine(returnCode);
	FlushBuffer();
	GetArg(num, 0, returnCode);
	if(StrCompare(num,"02") != 0) {
		MemPtrFree(returnCode);
		MemPtrFree(num);
		NetLibSocketShutdown(gNetRefNum,socket,netSocketDirBoth,10*SysTicksPerSecond(),&error);
    	NetLibSocketClose(gNetRefNum,socket,10*SysTicksPerSecond(),&error);
		return false;
	}
	else {
			GetArg(num,1,returnCode);
			if(StrCompare(num," 00") != 0) {
				MemPtrFree(returnCode);
				MemPtrFree(num);
				NetLibSocketShutdown(gNetRefNum,socket,netSocketDirBoth,10*SysTicksPerSecond(),&error);
    			NetLibSocketClose(gNetRefNum,socket,10*SysTicksPerSecond(),&error);
    			return false;
			}
	}
	
	MemPtrFree(returnCode);
	MemPtrFree(num);
	return true;

	
}










///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	Logout()
//
//	Log users out and return to login form
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void Logout() {
	//Err error;
	SendLine("11");
	//NetLibSocketShutdown(gNetRefNum,socket,netSocketDirBoth,10*SysTicksPerSecond(),&error);
    //NetLibSocketClose(gNetRefNum,socket,10*SysTicksPerSecond(),&error);
	FrmGotoForm(LoginForm);
}












///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	CheckForNewMessages()
//
//	See how many new messages await the user and
//		update label accordingly
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void CheckForNewMessages(Char * m) {
	
	char * response, * num;
	
	response = (Char *)MemPtrNew(64);
	num = (Char *)MemPtrNew(64);
	
	if(SendLine("13")) {
		GetLine(response);
		FlushBuffer();
		GetArg(num,0,response);
		if(StrCompare(num,"08") != 0) {
			StrCopy(m,"  ");
		}
		else {
			GetArg(num,1,response);
			StrCopy(m,num);
		}
	}
	else {
		StrCopy(m,"  ");
	}
	
	MemPtrFree(response);
	MemPtrFree(num);
	
	return;
		
}










///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	SendMessage()
//
//	Sends a text message to a user
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static Boolean SendMessage() {
	
	char *response = (char *)MemPtrNew(1024),
		 * num = (char *)MemPtrNew(1024),
		 * text = (char *)MemPtrNew(1024);
	FieldType *msgField = GetObjectPtr(SendMessageField);
	FieldType *usrField = GetObjectPtr(SendMessageToField);
	Boolean ret = false;
	
	StrCopy(text,"02~");
	StrCat(text,FldGetTextPtr(usrField));
	StrCat(text,"~");
	StrCat(text,FldGetTextPtr(msgField));
	StrCat(text,"\0");
	
	if(SendLine(text)) {
		GetLine(response);
		FlushBuffer();
		GetArg(num,0,response);
		if(StrCompare(num,"02") != 0);
		else{
			GetArg(num,1,response);
			if(StrCompare(num," 00") != 0);
			else {
				ret = true;
			}
		}
	}
	
	MemPtrFree(text);
	MemPtrFree(response);
	MemPtrFree(num);
	return ret;
	
	
	
}







///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	GetMessages()
//
//	Build a list of unread messages
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void GetMessages(ListType *list) {
	
	Char **itemList;
	UInt16 numItems;
	int counter = 0, i;
	char * response, * num;
	
	response = (Char *)MemPtrNew(1024);
	num = (Char *)MemPtrNew(1024);
	
	//Get number of checks first
	SendLine("04");
	GetLine(response);
	GetArg(num,0,response);
	while(StrCompare(num,"01") != 0) {
		counter++;
		GetLine(response);
		GetLine(response);
		GetArg(num,0,response);
	}
	GetLine(response);
	
	//Now set up list elements
	numItems = counter;
	if(counter == 0) {
		FlushBuffer();
		MemPtrFree(response);
		MemPtrFree(num);
		return;
	}
	itemList = (Char **) MemPtrNew(numItems * sizeof(Char *));
	gMsgArray = itemList;
	gMsgNum = numItems;
	
	//Fill the list with the appropriate items
	SendLine("04");
	for(i = 0 ; i < numItems ; i++) {
		itemList[i] = (Char *) MemPtrNew(1024);
		GetLine(response);
		GetArg(num,1,response);
		StrPrintF(itemList[i], num);
		StrCat(itemList[i], " ~>~ ");
		GetArg(num,2,response);
		StrCat(itemList[i],num);
		GetLine(response);	
	}
	LstSetListChoices(list, itemList, numItems);
	FlushBuffer();
	
	MemPtrFree(response);
	MemPtrFree(num);
	
	return;
	
	
}










///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	RemoveMessage()
//
//	Removes the selected Message from Inbox
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void RemoveMessage() {
	
	char * response = (char *)MemPtrNew(16),
		 * text = (char *)MemPtrNew(16);
	UInt32 msgID;
		 
	ListType *msgList = GetObjectPtr(InboxList);
	msgID = LstGetSelection(msgList);
	
	StrCopy(text,"12~");
	StrCat(text,StrIToA(response,msgID));
	StrCat(text,"\0");
	
	if(SendLine(text)) {
		GetLine(response);
		FlushBuffer();
	}
	
	MemPtrFree(response);
	MemPtrFree(text);
	
	FrmCustomAlert(AlertAlert, "Message has been removed.", NULL, NULL);
	FrmGotoForm(InboxForm);
	return;
	
}
	











///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	GetMOTD()
//
//	Get the MOTD from the server and set the field
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void GetMOTD() {
	
	char * response, * num;
	MemHandle newH = MemHandleNew(1024);
	FieldType *msgField = GetObjectPtr(MOTDField);
	char * s;
	
	
	response = (Char *)MemPtrNew(1024);
	num = (Char *)MemPtrNew(1024);
	
	if(SendLine("03")) {
		GetLine(response);
		FlushBuffer();
		GetArg(num,0,response);
		if(StrCompare(num,"04") != 0);
		else{
			s = (Char *)MemHandleLock(newH);
			GetArg(s,1,response);
			MemHandleUnlock(newH);
			FldSetTextHandle(msgField,newH);
			MemPtrFree(response);
			MemPtrFree(num);
			return;
		}
	}
	
	MemPtrFree(response);
	MemPtrFree(num);
	return;
}	










///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	OpenNewCheck()
//
//	Opens a new check with a reference number
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void OpenNewCheck() {
	
	char * response, * num, * send;
	
	response = (Char *)MemPtrNew(32);
	num = (Char *)MemPtrNew(32);
	send = (Char*)MemPtrNew(32);
	
	StrCopy(send,"05~");
	StrCat(send,CHECKNUM);
	SendLine(send);
	
	GetLine(response);
	FlushBuffer();
	GetArg(num, 0, response);
	if(StrCompare(num,"02") != 0) {
		FrmCustomAlert(AlertAlert, "Error: Could not open check with that number.", NULL, NULL);
	}
	else {
			GetArg(num,1,response);
			if(StrCompare(num," 00") != 0) {
    			FrmCustomAlert(AlertAlert, "Error: Could not open check with that number.", NULL, NULL);
			}
			else FrmCustomAlert(AlertAlert, "New check opened.", NULL, NULL);
	}
	
	MemPtrFree(send);
	MemPtrFree(response);
	MemPtrFree(num);
}









///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	CloseCheck()
//
//	Close a check with a given ID
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void CloseCheck() {
	
	char * response, * num, * send;
	
	response = (Char *)MemPtrNew(32);
	num = (Char *)MemPtrNew(32);
	send = (Char*)MemPtrNew(32);
	
	StrCopy(send,"07~");
	StrCat(send,CHECKNUM);
	SendLine(send);
	
	GetLine(response);
	FlushBuffer();
	GetArg(num, 0, response);
	if(StrCompare(num,"02") != 0) {
		FrmCustomAlert(AlertAlert, "Error: Could not close check.", NULL, NULL);
	}
	else {
			GetArg(num,1,response);
			if(StrCompare(num," 00") != 0) {
    			FrmCustomAlert(AlertAlert, "Error: Could not close check- no check with that ID exists.", NULL, NULL);
			}
			else FrmCustomAlert(AlertAlert, "Check closed.", NULL, NULL);
	}
	
	MemPtrFree(send);
	MemPtrFree(response);
	MemPtrFree(num);
	
	
}









///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	GetOpenChecks()
//
//	Builds a list of all open checks
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void GetOpenChecks(ListType *list) {
	
	Char **itemList;
	UInt16 numItems;
	int counter = 0, i;
	char * response, * num;
	
	response = (Char *)MemPtrNew(32);
	num = (Char *)MemPtrNew(32);
	
	//Get number of checks first
	SendLine("10");
	GetLine(response);
	GetArg(num,0,response);
	while(StrCompare(num,"01") != 0) {
		counter++;
		GetLine(response);
		GetLine(response);
		GetArg(num,0,response);
	}
	GetLine(response);
	
	//Now set up list elements
	numItems = counter;
	if(counter == 0) {
		FlushBuffer();
		MemPtrFree(response);
		MemPtrFree(num);
		return;
	}
	itemList = (Char **) MemPtrNew(numItems * sizeof(Char *));
	gOrdersArray = itemList;
	gOrdersNum = numItems;
	
	//Fill the list with the appropriate items
	SendLine("10");
	for(i = 0 ; i < numItems ; i++) {
		itemList[i] = (Char *) MemPtrNew(12);
		GetLine(response);
		GetArg(num,1,response);
		StrPrintF(itemList[i], num);
		GetLine(response);	
	}
	LstSetListChoices(list, itemList, numItems);
	FlushBuffer();
	
	MemPtrFree(response);
	MemPtrFree(num);
	
	return;
}







///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	GetCompleteItemList()
//
//	Returns all items available to order
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void GetCompleteItemList(ListType *list) {
	
	Char **itemList;
	UInt16 numItems;
	int counter = 0, i;
	char * response, * num;
	
	response = (Char *)MemPtrNew(1024);
	num = (Char *)MemPtrNew(1024);
	
	//Get number of checks first
	SendLine("08");
	GetLine(response);
	GetArg(num,0,response);
	while(StrCompare(num,"01") != 0) {
		counter++;
		GetLine(response);
		GetLine(response);
		GetArg(num,0,response);
	}
	GetLine(response);
	
	//Now set up list elements
	numItems = counter;
	//numItems = 3;
	itemList = (Char **) MemPtrNew(numItems * sizeof(Char *));
	gItemsArray = itemList;
	gItemsNum = numItems;
	
	//Fill the list with the appropriate items
	SendLine("08");
	for(i = 0 ; i < numItems ; i++) {
		itemList[i] = (Char *) MemPtrNew(1024);
		GetLine(response);
		GetArg(num,5,response);
		StrPrintF(itemList[i], num);
		StrCat(itemList[i], "~  ");
		GetArg(num,2,response);
		StrCat(itemList[i], num);
		StrCat(itemList[i],"~   ");
		GetArg(num,3,response);
		StrCat(itemList[i], num);
		StrCat(itemList[i],"~");
		GetArg(num,1,response);
		StrCat(itemList[i], num);
		GetLine(response);	
	}
	LstSetListChoices(list, itemList, numItems);
	FlushBuffer();
	
	MemPtrFree(response);
	MemPtrFree(num);
	
	return;
	
	
}








///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	OrderItem()
//
//	Order selected item to current checl
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void OrderItem() {
	
	char * response, * num, * send;
	ListType *iList;
	FieldType *modFld;
	
	response = (Char *)MemPtrNew(32);
	num = (Char *)MemPtrNew(32);
	send = (Char*)MemPtrNew(1024);
	
	iList = GetObjectPtr(OrderItemList);
	modFld = GetObjectPtr(OrderItemField);
	
	StrCopy(send,"06~");
	StrCat(send,CHECKNUM);
	StrCat(send,"~");
	GetArg(num,3,LstGetSelectionText(iList,LstGetSelection(iList)));
	StrCat(send,num);
	StrCat(send,"~");
	if(FldGetTextLength(modFld) > 0) StrCat(send,FldGetTextPtr(modFld));
	StrCat(send,"\0");
	
	SendLine(send);
	GetLine(response);
	FlushBuffer();
	GetArg(num, 0, response);
	if(StrCompare(num,"02") != 0) {
		FrmCustomAlert(AlertAlert, "Error: Could not order that item.", NULL, NULL);
	}
	else {
			GetArg(num,1,response);
			if(StrCompare(num," 00") != 0) {
    			FrmCustomAlert(AlertAlert, "Error: Could not order that item.", NULL, NULL);
			}
			else FrmCustomAlert(AlertAlert, "Item has been ordered", NULL, NULL);
	}
	
	MemPtrFree(send);
	MemPtrFree(response);
	MemPtrFree(num);
	
	
	
}














///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	GetItemsFromCheck()
//
//	Get a list of items currently on this check
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////


static void GetItemFromCheck(ListType *list) {
	
	Char **itemList;
	UInt16 numItems;
	int counter = 0, i;
	char * response, * num, *text;
	
	response = (Char *)MemPtrNew(1024);
	num = (Char *)MemPtrNew(1024);
	text = (Char *)MemPtrNew(16);
	
	//Get number of checks first
	StrCopy(text,"09~");
	StrCat(text,CHECKNUM);
	StrCat(text,"\0");
	SendLine(text);
	GetLine(response);
	GetArg(num,0,response);
	while(StrCompare(num,"01") != 0) {
		counter++;
		GetLine(response);
		GetLine(response);
		GetArg(num,0,response);
	}
	GetLine(response);
	
	//Now set up list elements
	numItems = counter;
	if(counter == 0) {
		FlushBuffer();
		MemPtrFree(response);
		MemPtrFree(num);
		MemPtrFree(text);
		return;
	}
	itemList = (Char **) MemPtrNew(numItems * sizeof(Char *));
	gCheckArray = itemList;
	gCheckNum = numItems;
	
	//Fill the list with the appropriate items
	SendLine(text);
	for(i = 0 ; i < numItems ; i++) {
		itemList[i] = (Char *) MemPtrNew(1024);
		GetLine(response);
		GetArg(num,5,response);
		StrPrintF(itemList[i], num);
		StrCat(itemList[i], "~");
		
		GetArg(num,2,response);
		StrCat(itemList[i], num);
		StrCat(itemList[i],"    ~");
		
		GetArg(num,4,response);
		StrCat(itemList[i], num);
		StrCat(itemList[i], "\0");
		GetLine(response);	
	}
	LstSetListChoices(list, itemList, numItems);
	FlushBuffer();
	
	MemPtrFree(response);
	MemPtrFree(num);
	MemPtrFree(text);
	
	return;
	
	
	
}








///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	GetTotal()
//
//	Get the total price for this check so far
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void GetTotal(char *m) {
	
	char * response, * num, * text;
	
	response = (Char *)MemPtrNew(64);
	num = (Char *)MemPtrNew(64);
	text = (Char *)MemPtrNew(64);
	
	StrCopy(text,"14~");
	StrCat(text,CHECKNUM);
	
	if(SendLine(text)) {
		GetLine(response);
		FlushBuffer();
		GetArg(num,0,response);
		if(StrCompare(num,"09") != 0) {
			StrCopy(m,"  ");
		}
		else {
			GetArg(num,1,response);
			StrCopy(m,num);
		}
	}
	else {
		StrCopy(m,"000.00");
	}
	
	MemPtrFree(response);
	MemPtrFree(num);
	MemPtrFree(text);
	
	return;
	
	
	
	
	
	
	
	
	
}








///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	Form Handlers
//
//	These functions handle the events for each of
//	the main forms
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static Boolean HandleLogin(EventType *event) {
	
	Boolean   handled = false;
    FormType  *form;
    FieldType *user, *pass;
    ControlType *checkbox;
    FieldType *ip1, *ip2, *ip3, *ip4, *port;
    
    switch (event->eType) {
	    
	    case frmOpenEvent:
            form = FrmGetActiveForm();	
            FrmDrawForm(form);
            FrmSetFocus(form, FrmGetObjectIndex(form,LoginUsernameField));
            handled = true;
            break;
            
        case ctlSelectEvent:
        	switch(event->data.ctlSelect.controlID) {
	        	
	        	case LoginButton:
	        		user = GetObjectPtr(LoginUsernameField);
	        		pass = GetObjectPtr(LoginPasswordField);
	        		checkbox = GetObjectPtr(LoginOverrideIPCheckbox);
	        		IPAddr = (Char *)MemPtrNew(16);
	        		
	        		if(CtlGetValue(checkbox)) {  //Override the IP address
	        			ip1 = GetObjectPtr(LoginOverrideIPField1);
	        			ip2 = GetObjectPtr(LoginOverrideIPField2);
	        			ip3 = GetObjectPtr(LoginOverrideIPField3);
	        			ip4 = GetObjectPtr(LoginOverrideIPField4);
	        			port = GetObjectPtr(LoginPortField);
	        			if(FldGetTextLength(ip1) <= 0 ||
	        			   FldGetTextLength(ip2) <= 0 ||
	        			   FldGetTextLength(ip3) <= 0 ||
	        			   FldGetTextLength(ip4) <= 0 ||
	        			   FldGetTextLength(port) <= 0)
	        			   FrmCustomAlert(AlertAlert, "Please enter an IP and port.", NULL, NULL);
	        			else {   
	        				StrCopy(IPAddr, FldGetTextPtr(ip1));
	        				StrCat(IPAddr, ".");
	        				StrCat(IPAddr, FldGetTextPtr(ip2));
	        				StrCat(IPAddr, ".");
	        				StrCat(IPAddr, FldGetTextPtr(ip3));
	        				StrCat(IPAddr, ".");
	        				StrCat(IPAddr, FldGetTextPtr(ip4));
	        				StrCat(IPAddr, "\0");
	        				PORT = StrAToI(FldGetTextPtr(port));
	        				override = true;
	        			}
	        		}
	        		else override = false;
	        		
	        		if(FldGetTextLength(user) <= 0 || FldGetTextLength(pass) <= 0) //Username or password field is empty
		        		FrmCustomAlert(AlertAlert, "Please supply and username and password.", NULL, NULL);
		        	else {
			        	if( Login() ) {
			        		FrmGotoForm(MainMenuForm);
			        	}
			        	else FrmCustomAlert(AlertAlert, "Error: Login failed.", NULL, NULL);
		        	}
		        	
		        	MemPtrFree(IPAddr);
	        		handled = true;
	        		break;
	        		
        	}
        	break;

        default:
            break;
    }

    return handled;
}

///////////////////////////////////////////////////

static Boolean HandleMainMenu(EventType *event) {
	
	Boolean   handled = false;
    FormType  *form;
    FieldType *checkID;
    ControlType *msgLabel;
    
    switch (event->eType) {
	    
	    case frmOpenEvent:
            form = FrmGetActiveForm();
            numMsgs = MemPtrNew(10);
            msgLabel = GetObjectPtr(MainMenuNewMessagesNumLabel);
            CheckForNewMessages(numMsgs);
            CtlSetLabel(msgLabel,numMsgs);
            FrmDrawForm(form);
            FrmSetFocus(form, FrmGetObjectIndex(form,MainMenuCheckIDField));
            handled = true;
            break;
            
        case frmCloseEvent:
        	MemPtrFree(numMsgs);
        	break;
            
        case ctlSelectEvent:
        	switch(event->data.ctlSelect.controlID) {
	        	
	        	case MainMenuViewMOTDButton:
	        		FrmGotoForm(MOTDForm);
	        		handled = true;
	        		break;
	        		
	       		case MainMenuCheckMessagesButton:
	       			FrmGotoForm(InboxForm);
	        		handled = true;
	        		break;
	        	
	        	case MainMenuOpenNewCheckButton:
	        		checkID = GetObjectPtr(MainMenuCheckIDField);
	        		if(FldGetTextLength(checkID) <= 0) //Username or password field is empty
		        		FrmCustomAlert(AlertAlert, "You must enter a check ID before opening it.", NULL, NULL);
	        		else{
	        			CHECKNUM = (Char *)MemPtrNew(16);
	        			StrCopy(CHECKNUM,FldGetTextPtr(checkID));
	        			StrCat(CHECKNUM,"\0");
	        			OpenNewCheck();
	        			MemPtrFree(CHECKNUM);
	        		}
	        		handled = true;
	        		break;
	        		
	        	case MainMenuCloseCheckButton:
	        		checkID = GetObjectPtr(MainMenuCheckIDField);
	        		if(FldGetTextLength(checkID) <= 0) //Username or password field is empty
		        		FrmCustomAlert(AlertAlert, "You must enter a check ID before closing it.", NULL, NULL);
	        		else{
	        			CHECKNUM = (Char *)MemPtrNew(16);
	        			StrCopy(CHECKNUM,FldGetTextPtr(checkID));
	        			StrCat(CHECKNUM,"\0");
	        			CloseCheck();
	        			MemPtrFree(CHECKNUM);
	        		}
	        		handled = true;
	        		break;
	        		
	        	case MainMenuViewCheckButton:
	        		checkID = GetObjectPtr(MainMenuCheckIDField);
	        		if(FldGetTextLength(checkID) <= 0) //Username or password field is empty
		        		FrmCustomAlert(AlertAlert, "You must enter a check ID before viewing it.", NULL, NULL);
	        		else{
	        			CHECKNUM = (Char *)MemPtrNew(16);
	        			StrCopy(CHECKNUM,FldGetTextPtr(checkID));
	        			StrCat(CHECKNUM,"\0");
	        			FrmGotoForm(CheckForm);
	        		}
	        		handled = true;
	        		break;
	        		
	        	case MainMenuViewOpenChecksButton:
	        		FrmGotoForm(OpenChecksForm);
	        		handled = true;
	        		break;
	        	
	        	case MainMenuLogoutButton:
	        		Logout();
	        		handled = true;
	        		break;
	        		
        	}
        	break;

        default:
            break;
    }

    return handled;
}

///////////////////////////////////////////////////

static Boolean HandleInbox(EventType *event) {
	
	Boolean   handled = false;
    FormType  *form;
    ListType  *mList;
    char *s;
    
    switch (event->eType) {
	    
	    case frmOpenEvent:
            form = FrmGetActiveForm();
            mList = GetObjectPtr(OpenChecksList);
            GetMessages(mList);
            LstSetSelection(mList, 0);
            FrmDrawForm(form);	
            handled = true;
            break;
            
        case frmCloseEvent:
       		if (gMsgArray) {
          		UInt16 i;
          		for (i=0; i<gMsgNum; i++)
             		MemPtrFree((MemPtr) gMsgArray[i]);
          		MemPtrFree((MemPtr) gMsgArray);
          		gMsgArray=0;
       		}
       		break;
        
        case ctlSelectEvent:
        	switch(event->data.ctlSelect.controlID) {
	        	
	        	case InboxNewButton:
	        		FrmGotoForm(SendMessageForm);
	        		handled = true;
	        		break;
	        		
	        	case InboxViewButton:
	        		mList = GetObjectPtr(OpenChecksList);
	        		if(LstGetNumberOfItems(mList))
	        			FrmCustomAlert(AlertAlert, LstGetSelectionText(mList,LstGetSelection(mList)), NULL, NULL);
	        		else FrmCustomAlert(AlertAlert, "Error: Please select a message first.", NULL, NULL);
	        		handled = true;
	        		break;
	        		
	        	case InboxReplyButton:
	        		mList = GetObjectPtr(OpenChecksList);
	        		if( LstGetNumberOfItems(mList) ) {
	        			replyTo = (Char*)MemPtrNew(16);
	        			s = (Char*)MemPtrNew(1024);
	        			StrCopy(s,LstGetSelectionText(mList,LstGetSelection(mList)));
	        			GetArg(replyTo, 0, s);
	        			MemPtrFree(s);
	        			FrmGotoForm(SendMessageForm);
	        		}
	        		else FrmCustomAlert(AlertAlert, "Error: Please select a message first.", NULL, NULL);
	        		handled = true;
	        		break;
	        		
	        	case InboxDeleteButton:
	        		mList = GetObjectPtr(OpenChecksList);
	        		if(LstGetSelection(mList) != noListSelection)
	        			RemoveMessage();
	        		else FrmCustomAlert(AlertAlert, "Error: Please select a message first.", NULL, NULL);
	        		handled = true;
	        		break;
	        		
	        	case InboxBackButton:
	        		FrmGotoForm(MainMenuForm);
	        		handled = true;
	        		break;
	        	
        	}
        	break;

        default:
            break;
    }

    return handled;
}

///////////////////////////////////////////////////

static Boolean HandleMOTD(EventType *event) {
	
	Boolean   handled = false;
    FormType  *form;
    
    switch (event->eType) {
	    
	    case frmOpenEvent:
            form = FrmGetActiveForm();	
            GetMOTD();
            FrmDrawForm(form);	
            handled = true;
            break;
            
        case ctlSelectEvent:
        	switch(event->data.ctlSelect.controlID) {
	        	
	        	case MOTDBackButton:
	        		FrmGotoForm(MainMenuForm);
	        		handled = true;
	        		break;
	        	
        	}
        	break;

        default:
            break;
    }

    return handled;
}

///////////////////////////////////////////////////

static Boolean HandleCheck(EventType *event) {
	
	Boolean   handled = false;
    FormType  *form;
    ListType *cList;
    ControlType *checkID;
    ControlType *pTot;
    
    switch (event->eType) {
	    
	    case frmOpenEvent:
            form = FrmGetActiveForm();
            checkID = GetObjectPtr(CheckIDLabelNum);
            pTot = GetObjectPtr(CheckTotalLabelNum);
            CtlSetLabel(checkID,CHECKNUM);
            checkTotal = MemPtrNew(16);
            GetTotal(checkTotal);
            CtlSetLabel(pTot,checkTotal);
            cList = GetObjectPtr(CheckList);
            GetItemFromCheck(cList);	
            if(gCheckArray) LstSetSelection(cList, 0);
            FrmDrawForm(form);	
            handled = true;
            break;
            
        case frmCloseEvent:
        	MemPtrFree(checkTotal);
       		if (gCheckArray) {
          		UInt16 i;
          		for (i=0; i<gCheckNum; i++)
             		MemPtrFree((MemPtr) gCheckArray[i]);
          		MemPtrFree((MemPtr) gCheckArray);
          		gCheckArray=0;
       		}
       		break;
            
        case ctlSelectEvent:
        	switch(event->data.ctlSelect.controlID) {
	        	
	        	case CheckAddItemButton:
	        		FrmGotoForm(OrderItemForm);
	        		handled = true;
	        		break;
	        		
	        	case CheckBackButton:
	        		MemPtrFree(CHECKNUM);
	        		FrmGotoForm(MainMenuForm);
	        		handled = true;
	        		break;
	        	
        	}
        	break;

        default:
            break;
    }

    return handled;
}

///////////////////////////////////////////////////

static Boolean HandleOpenChecks(EventType *event) {
	
	Boolean   handled = false;
    FormType  *form;
    ListType  *oList;
    
    switch (event->eType) {
	    
	    case frmOpenEvent:
            form = FrmGetActiveForm();	
            oList = GetObjectPtr(OpenChecksList);
            GetOpenChecks(oList);
            if(gOrdersArray) LstSetSelection(oList, 0);
            FrmDrawForm(form);	
            handled = true;
            break;
            
        case frmCloseEvent:
       		if (gOrdersArray) {
          		UInt16 i;
          		for (i=0; i<gOrdersNum; i++)
             		MemPtrFree((MemPtr) gOrdersArray[i]);
          		MemPtrFree((MemPtr) gOrdersArray);
          		gOrdersArray=0;
       		}
       		break;
       
        case ctlSelectEvent:
        	switch(event->data.ctlSelect.controlID) {
	        	
	        	case OpenChecksViewCheckButton:
	        		oList = GetObjectPtr(OpenChecksList);
	        		if( LstGetNumberOfItems(oList) ) {
	        			CHECKNUM = (Char *)MemPtrNew(16);
	        			StrCopy(CHECKNUM,LstGetSelectionText(oList,LstGetSelection(oList)));
	        			StrCat(CHECKNUM,"\0");
	        			FrmGotoForm(CheckForm);
	        		}
	        		else FrmCustomAlert(AlertAlert, "Error: Please select a check first", NULL, NULL);
	        		handled = true;
	        		break;
	        		
	        	case OpenChecksBackButton:
	        		FrmGotoForm(MainMenuForm);
	        		handled = true;
	        		break;
	        	
        	}
        	break;

        default:
            break;
    }

    return handled;
}

///////////////////////////////////////////////////

static Boolean HandleMessage(EventType *event) {
	
	Boolean   handled = false;
    FormType  *form;
    FieldType *toFld;
    FieldAttrType attr;
    
    switch (event->eType) {
	    
	    case frmOpenEvent:
            form = FrmGetActiveForm();	
            if(replyTo != NULL) {
            	toFld = GetObjectPtr(SendMessageToField);
            	FldSetTextPtr(toFld,replyTo);
            	FldGetAttributes(toFld, &attr);
            	attr.editable = 0;
            	FldSetAttributes(toFld, &attr);
            }
            FrmDrawForm(form);	
            FrmSetFocus(form, FrmGetObjectIndex(form,SendMessageToField));
            handled = true;
            break;
            
        case frmCloseEvent:
        	if(replyTo != NULL) MemPtrFree(replyTo);
        	replyTo = NULL;
        	toFld = GetObjectPtr(SendMessageToField);
        	FldGetAttributes(toFld, &attr);
            attr.editable = 1;
            FldSetAttributes(toFld, &attr);
        	break;
            
        case ctlSelectEvent:
        	switch(event->data.ctlSelect.controlID) {
	        	
	        	case SendMessageSendButton:
	        		if(SendMessage())
	        			FrmCustomAlert(AlertAlert, "Message Sent", NULL, NULL);
	        		else FrmCustomAlert(AlertAlert, "Message could not be sent", NULL, NULL);
	        		handled = true;
	        		break;
	        		
	        	case SendMessageBackButton:
	        		FrmGotoForm(InboxForm);
	        		handled = true;
	        		break;
	        		
        	}
        	break;

        default:
            break;
    }

    return handled;
}

///////////////////////////////////////////////////

static Boolean HandleOrder(EventType *event) {
	
	Boolean   handled = false;
    FormType  *form;
    ListType  *iList;
    char *desc;
    
    switch (event->eType) {
	    
	    case frmOpenEvent:
            form = FrmGetActiveForm();
            iList = GetObjectPtr(OrderItemList);
            GetCompleteItemList(iList);	
            if(gItemsArray) LstSetSelection(iList, noListSelection);
            FrmDrawForm(form);	
            handled = true;
            break;
            
        case frmCloseEvent:
       		if (gItemsArray) {
          		UInt16 i;
          		for (i=0; i<gItemsNum; i++)
             		MemPtrFree((MemPtr) gItemsArray[i]);
          		MemPtrFree((MemPtr) gItemsArray);
          		gItemsArray=0;
       		}
       		break;
       		
       	case popSelectEvent:
       		iList = GetObjectPtr(OrderItemList);
       		break;
            
        case ctlSelectEvent:
        	switch(event->data.ctlSelect.controlID) {
	        	
	        	case OrderItemViewDescriptionButton:
	        		iList = GetObjectPtr(OrderItemList);
	        		if(LstGetSelection(iList) != noListSelection){
	        			desc = MemPtrNew(1024);
	        			GetArg(desc,2,LstGetSelectionText(iList,LstGetSelection(iList)));
	        			FrmCustomAlert(AlertAlert, desc, NULL, NULL);
	        			MemPtrFree(desc);
	        		}
	        		else FrmCustomAlert(AlertAlert, "Error: Please select an item first", NULL, NULL);
	        		handled = true;
	        		break;
	        		
	        	case OrderItemOrderButton:
	        		OrderItem();
	        		handled = true;
	        		break;
	        		
	        	case OrderItemBackButton:
	        		FrmGotoForm(CheckForm);
	        		handled = true;
	        		break;
	        	
        	}
        	break;

        default:
            break;
    }

    return handled;
}

















///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	AppHandleEvent()
//
//	Initalize resources and setup even handlers
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static Boolean AppHandleEvent(EventType *event) {

    FormType  *form;
    UInt16    formID;
    Boolean   handled = false;


    if (event->eType == frmLoadEvent) {
        formID = event->data.frmLoad.formID;
        form = FrmInitForm(formID);				
        FrmSetActiveForm(form);					

        switch (formID) {
			
            case LoginForm:
                FrmSetEventHandler(form, HandleLogin);
                break;
                
            case MainMenuForm:
            	FrmSetEventHandler(form, HandleMainMenu);
                break;
                
            case InboxForm:
            	FrmSetEventHandler(form, HandleInbox);
                break;
                
            case MOTDForm:
            	FrmSetEventHandler(form, HandleMOTD);
                break;
                
            case CheckForm:
            	FrmSetEventHandler(form, HandleCheck);
                break;
                
            case OpenChecksForm:
            	FrmSetEventHandler(form, HandleOpenChecks);
                break;
                
            case SendMessageForm:
            	FrmSetEventHandler(form, HandleMessage);
                break;
                
            case OrderItemForm:
            	FrmSetEventHandler(form, HandleOrder);
                break;

            default:
                break;
                
        }
        handled = true;
    }

    return handled;
}


















///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	AppEventLoop()
//
//	Get events from the queue and send them to the
//		correct handler.
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

static void AppEventLoop(void) {

    EventType  event;
    Err        error;


    do {
        EvtGetEvent(&event, evtWaitForever);	
        
        if (! SysHandleEvent(&event))	
            if (! MenuHandleEvent(0, &event, &error))	
                if (! AppHandleEvent(&event))	
                    FrmDispatchEvent(&event);	
    }
    while (event.eType != appStopEvent);	
    
}















///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
// AppStart() and AppStop()
//
//	Setup and shutdown the application
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

//Open the net library
static Err AppStart(void) {
	UInt16 ifErrs;
	Err error = SysLibFind("Net.lib", &gNetRefNum);
	if(!error) {
		error = NetLibOpen(gNetRefNum, &ifErrs);
		if(!error) return errNone;
		else {
			if(ifErrs) return ifErrs;
			else return error;
		}
	}
	
	return error;
}

static void AppStop(void) {
	Err error;
	error = NetLibClose(gNetRefNum,true);
    FrmCloseAllForms();
}















///////////////////////////////////////////////////
///////////////////////////////////////////////////
//
//	PilotMain()
//
//	This is the main entry point for the program
//
///////////////////////////////////////////////////
///////////////////////////////////////////////////

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags) {

    Err  error;
    
    switch(cmd) {
    	
    	case sysAppLaunchCmdNormalLaunch:
    		//Start Program
			error = AppStart();	
			if (error)			
    			return error;	
    
    		//Goto login form and loop event handlers           
    		FrmGotoForm(LoginForm);
    		AppEventLoop();

    		//End Program
    		AppStop();	
    		break;
    	
    	default:
    		break;

    }
    
    return errNone;
}