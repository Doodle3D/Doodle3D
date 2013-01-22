#include "ofxSerial.h"
#include "ofUtils.h"
#include "ofTypes.h"

#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	#include <sys/ioctl.h>
	#include <getopt.h>
	#include <dirent.h>
#endif
#if defined( TARGET_LINUX )
	#include <linux/serial.h>
#endif
#if defined ( TARGET_OSX )
	#include <AvailabilityMacros.h>
	#if defined(MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
		#include <IOKit/serial/ioss.h>
	#endif
#endif

#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <algorithm>


//---------------------------------------------
#ifdef TARGET_WIN32
//---------------------------------------------

//------------------------------------
   // needed for serial bus enumeration:
   //4d36e978-e325-11ce-bfc1-08002be10318}
   DEFINE_GUID (GUID_SERENUM_BUS_ENUMERATOR2, 0x4D36E978, 0xE325,
   0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);
//------------------------------------

void ofxSerial::enumerateWin32Ports(){

    // thanks joerg for fixes...

	if (bPortsEnumerated == true) return;

	HDEVINFO hDevInfo = NULL;
	SP_DEVINFO_DATA DeviceInterfaceData;
	int i = 0;
	DWORD dataType, actualSize = 0;
	unsigned char dataBuf[MAX_PATH + 1];

	// Reset Port List
	nPorts = 0;
	// Search device set
	hDevInfo = SetupDiGetClassDevs((struct _GUID *)&GUID_SERENUM_BUS_ENUMERATOR2,0,0,DIGCF_PRESENT);
	if ( hDevInfo ){
      while (TRUE){
         ZeroMemory(&DeviceInterfaceData, sizeof(DeviceInterfaceData));
         DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
         if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInterfaceData)){
             // SetupDiEnumDeviceInfo failed
             break;
         }

         if (SetupDiGetDeviceRegistryPropertyA(hDevInfo,
             &DeviceInterfaceData,
             SPDRP_FRIENDLYNAME,
             &dataType,
             dataBuf,
             sizeof(dataBuf),
             &actualSize)){

			sprintf(portNamesFriendly[nPorts], "%s", dataBuf);
			portNamesShort[nPorts][0] = 0;

			// turn blahblahblah(COM4) into COM4

            char *   begin    = NULL;
            char *   end    = NULL;
            begin          = strstr((char *)dataBuf, "COM");


            if (begin)
                {
                end          = strstr(begin, ")");
                if (end)
                    {
                      *end = 0;   // get rid of the )...
                      strcpy(portNamesShort[nPorts], begin);
                }
                if (nPorts++ > MAX_SERIAL_PORTS)
                        break;
            }
         }
            i++;
      }
   }
   SetupDiDestroyDeviceInfoList(hDevInfo);

   bPortsEnumerated = false;
}


//---------------------------------------------
#endif
//---------------------------------------------



//----------------------------------------------------------------
ofxSerial::ofxSerial(){

	//---------------------------------------------
	#ifdef TARGET_WIN32
	//---------------------------------------------
		nPorts 				= 0;
		bPortsEnumerated 	= false;

		portNamesShort = new char * [MAX_SERIAL_PORTS];
		portNamesFriendly = new char * [MAX_SERIAL_PORTS];
		for (int i = 0; i < MAX_SERIAL_PORTS; i++){
			portNamesShort[i] = new char[10];
			portNamesFriendly[i] = new char[MAX_PATH];
		}
	//---------------------------------------------
	#endif
	//---------------------------------------------
	bVerbose = false;
	bInited = false;
}

//----------------------------------------------------------------
ofxSerial::~ofxSerial(){

	close();



	//---------------------------------------------
	#ifdef TARGET_WIN32
	//---------------------------------------------
		nPorts 				= 0;
		bPortsEnumerated 	= false;

		for (int i = 0; i < MAX_SERIAL_PORTS; i++) {
			delete [] portNamesShort[i];
			delete [] portNamesFriendly[i];
		}
		delete [] portNamesShort;
		delete [] portNamesFriendly;

	//---------------------------------------------
	#endif
	//---------------------------------------------

	bVerbose = false;
	bInited = false;
}

//----------------------------------------------------------------
static bool isDeviceArduino( ofSerialDeviceInfo & A ){
	//TODO - this should be ofStringInString
	return ( strstr(A.getDeviceName().c_str(), "usbserial") != NULL );
}

//----------------------------------------------------------------
void ofxSerial::buildDeviceList(){

	deviceType = "serial";
	devices.clear();

	vector <string> prefixMatch;

	#ifdef TARGET_OSX
		prefixMatch.push_back("cu.");
		prefixMatch.push_back("tty.");
	#endif
	#ifdef TARGET_LINUX
		prefixMatch.push_back("ttyS");
		prefixMatch.push_back("ttyUSB");
		prefixMatch.push_back("rfc");
	#endif


	#if defined( TARGET_OSX ) || defined( TARGET_LINUX )

	DIR *dir;
	struct dirent *entry;
	dir = opendir("/dev");

	string deviceName	= "";
	int deviceCount		= 0;

	if (dir == NULL){
		ofLog(OF_LOG_ERROR,"ofxSerial: error listing devices in /dev");
	} else {
		//for each device
		while((entry = readdir(dir)) != NULL){
			deviceName = (char *)entry->d_name;

			//we go through the prefixes
			for(int k = 0; k < (int)prefixMatch.size(); k++){
				//if the device name is longer than the prefix
				if( deviceName.size() > prefixMatch[k].size() ){
					//do they match ?
					if( deviceName.substr(0, prefixMatch[k].size()) == prefixMatch[k].c_str() ){
						devices.push_back(ofSerialDeviceInfo("/dev/"+deviceName, deviceName, deviceCount));
						deviceCount++;
						break;
					}
				}
			}
		}
		closedir(dir);
	}

	#endif

	//---------------------------------------------
	#ifdef TARGET_WIN32
	//---------------------------------------------
	enumerateWin32Ports();
	ofLogNotice() << "ofxSerial: listing devices (" << nPorts << " total)";
	for (int i = 0; i < nPorts; i++){
		//NOTE: we give the short port name for both as that is what the user should pass and the short name is more friendly
		devices.push_back(ofSerialDeviceInfo(string(portNamesShort[i]), string(portNamesShort[i]), i));
	}
	//---------------------------------------------
	#endif
    //---------------------------------------------

//DISABLED because deviceID is protected
//	//here we sort the device to have the aruino ones first.
//	partition(devices.begin(), devices.end(), isDeviceArduino);
//	//we are reordering the device ids. too!
//	for(int k = 0; k < (int)devices.size(); k++){
//		devices[k].deviceID = k;
//	}

	bHaveEnumeratedDevices = true;
}


//----------------------------------------------------------------
void ofxSerial::listDevices(){
	buildDeviceList();
	for(int k = 0; k < (int)devices.size(); k++){
		ofLogNotice() << "[" << devices[k].getDeviceID() << "] = "<< devices[k].getDeviceName().c_str();
	}
}

//----------------------------------------------------------------
vector <ofSerialDeviceInfo> ofxSerial::getDeviceList(){
	buildDeviceList();
	return devices;
}

//----------------------------------------------------------------
void ofxSerial::enumerateDevices(){
	listDevices();
}

//----------------------------------------------------------------
void ofxSerial::close(){

	//---------------------------------------------
	#ifdef TARGET_WIN32
	//---------------------------------------------
		if (bInited){
			SetCommTimeouts(hComm,&oldTimeout);
			CloseHandle(hComm);
			hComm 		= INVALID_HANDLE_VALUE;
			bInited 	= false;
		}
	//---------------------------------------------
    #else
    //---------------------------------------------
    	if (bInited){
    		tcsetattr(fd,TCSANOW,&oldoptions);
    		::close(fd);
    	}
    	// [CHECK] -- anything else need to be reset?
    //---------------------------------------------
    #endif
    //---------------------------------------------

}

//----------------------------------------------------------------
bool ofxSerial::setup(){
	return setup(0,9600);		// the first one, at 9600 is a good choice...
}

//----------------------------------------------------------------
bool ofxSerial::setup(int deviceNumber, int baud){

	buildDeviceList();
	if( deviceNumber < (int)devices.size() ){
		return setup(devices[deviceNumber].getDevicePath(), baud);
	}else{
		ofLog(OF_LOG_ERROR,"ofxSerial: could not find device %i - only %i devices found", deviceNumber, devices.size());
		return false;
	}

}

//----------------------------------------------------------------
bool ofxSerial::setup(string portName, int baud){

	bInited = false;

	//---------------------------------------------
	#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	//---------------------------------------------

		//lets account for the name being passed in instead of the device path
		if( portName.size() > 5 && portName.substr(0, 5) != "/dev/" ){
			portName = "/dev/" + portName;
		}

	    ofLog(OF_LOG_NOTICE,"ofxSerialInit: opening port %s @ %d bps", portName.c_str(), baud);
		fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
		if(fd == -1){
			ofLog(OF_LOG_ERROR,"ofxSerial: unable to open port %s", portName.c_str());
			return false;
		}

		bool setCustomBaud = false;
		struct termios options;
		tcgetattr(fd,&oldoptions);
		options = oldoptions;
		#ifdef TARGET_LINUX
			// configure port not to use a custom speed
			struct serial_struct ss;
			ioctl(fd, TIOCGSERIAL, &ss);
			ss.flags &= ~ASYNC_SPD_MASK;
			ss.custom_divisor = (ss.baud_base + (baud / 2)) / baud;
			ioctl(fd, TIOCSSERIAL, &ss);
		#endif
		switch(baud){
		   case 300: 	cfsetispeed(&options,B300);
						cfsetospeed(&options,B300);
						break;
		   case 1200: 	cfsetispeed(&options,B1200);
						cfsetospeed(&options,B1200);
						break;
		   case 2400: 	cfsetispeed(&options,B2400);
						cfsetospeed(&options,B2400);
						break;
		   case 4800: 	cfsetispeed(&options,B4800);
						cfsetospeed(&options,B4800);
						break;
		   case 9600: 	cfsetispeed(&options,B9600);
						cfsetospeed(&options,B9600);
						break;
		   case 14400: 	cfsetispeed(&options,B14400);
						cfsetospeed(&options,B14400);
						break;
		   case 19200: 	cfsetispeed(&options,B19200);
						cfsetospeed(&options,B19200);
						break;
		   case 28800: 	cfsetispeed(&options,B28800);
						cfsetospeed(&options,B28800);
						break;
		   case 38400: 	cfsetispeed(&options,B38400);
						cfsetospeed(&options,B38400);
						break;
		   case 57600:  cfsetispeed(&options,B57600);
						cfsetospeed(&options,B57600);
						break;
		   case 115200: cfsetispeed(&options,B115200);
						cfsetospeed(&options,B115200);
						break;
		   #ifdef B230400
			   case 230400: cfsetispeed(&options,B230400);
							cfsetospeed(&options,B230400);
							break;
		   #endif
		   #ifdef B460800
			   case 460800: cfsetispeed(&options,B460800);
							cfsetospeed(&options,B460800);
							break;
		   #endif
		   #ifdef B500000
			   case 500000: cfsetispeed(&options,B500000);
							cfsetospeed(&options,B500000);
							break;
		   #endif
		   #ifdef B576000
			   case 576000: cfsetispeed(&options,B576000);
							cfsetospeed(&options,B576000);
							break;
		   #endif
		   #ifdef B921600
			   case 921600: cfsetispeed(&options,B921600);
							cfsetospeed(&options,B921600);
							break;
		   #endif
		   #ifdef B1000000
			   case 1000000: cfsetispeed(&options,B1000000);
							 cfsetospeed(&options,B1000000);
							 break;
		   #endif
		   #ifdef B1152000
			   case 1152000: cfsetispeed(&options,B1152000);
							 cfsetospeed(&options,B1152000);
							 break;
		   #endif
		   #ifdef B1500000
			   case 1500000: cfsetispeed(&options,B1500000);
							 cfsetospeed(&options,B1500000);
							 break;
		   #endif
		   #ifdef B2000000
			   case 2000000: cfsetispeed(&options,B2000000);
							 cfsetospeed(&options,B2000000);
							 break;
		   #endif
		   #ifdef B2500000
			   case 2500000: cfsetispeed(&options,B2500000);
							 cfsetospeed(&options,B2500000);
							 break;
		   #endif
		   #ifdef B3000000
			   case 3000000: cfsetispeed(&options,B3000000);
							 cfsetospeed(&options,B3000000);
							 break;
		   #endif
		   #ifdef B3500000
			   case 3500000: cfsetispeed(&options,B3500000);
							 cfsetospeed(&options,B3500000);
							 break;
		   #endif
		   #ifdef B4000000
			   case 4000000: cfsetispeed(&options,B4000000);
							 cfsetospeed(&options,B4000000);
							 break;
		   #endif
			default:
						#ifdef TARGET_LINUX
							//Set the speed now to 38400, and after configuring the port set the custom baudrate
							cfsetispeed(&options,B38400);
							cfsetospeed(&options,B38400);
							setCustomBaud = true;
						#elif defined( TARGET_OSX) && defined( IOSSIOSPEED )
							//Set the speed now to 9600, and after configuring the port set the custom baudrate
							cfsetispeed(&options,B9600);
							cfsetospeed(&options,B9600);
							setCustomBaud = true;
						#else
							cfsetispeed(&options,B9600);
							cfsetospeed(&options,B9600);
							ofLog(OF_LOG_ERROR,"ofxSerialInit: cannot set %i baud setting baud to 9600", baud);
						#endif
						break;
		}

		options.c_cflag |= (CLOCAL | CREAD);
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS8;
		tcsetattr(fd,TCSANOW,&options);

		if (setCustomBaud)
		{
			#ifdef TARGET_LINUX
				// configure port to use custom speed
				ioctl(fd, TIOCGSERIAL, &ss);
				ss.flags = (ss.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
				ss.custom_divisor = (ss.baud_base + (baud / 2)) / baud;
				ioctl(fd, TIOCSSERIAL, &ss);
			#elif defined( TARGET_OSX) && defined( IOSSIOSPEED )
				speed_t speed = baud;
				if (ioctl(fd, IOSSIOSPEED, &speed) == -1)
				{
					ofLog(OF_LOG_ERROR,"ofxSerialInit: cannot set %i baud setting baud to 9600", baud);
				}
			#endif
		}

		bInited = true;
		ofLog(OF_LOG_NOTICE,"sucess in opening serial connection");

	    return true;
	//---------------------------------------------
    #endif
    //---------------------------------------------


    //---------------------------------------------
	#ifdef TARGET_WIN32
	//---------------------------------------------

	// open the serial port:
	// "COM4", etc...

	hComm=CreateFileA(portName.c_str(),GENERIC_READ|GENERIC_WRITE,0,0,
					OPEN_EXISTING,0,0);

	if(hComm==INVALID_HANDLE_VALUE){
		ofLog(OF_LOG_ERROR,"ofxSerial: unable to open port");
		return false;
	}


	// now try the settings:
	COMMCONFIG cfg;
	DWORD cfgSize;
	char  buf[80];

	cfgSize=sizeof(cfg);
	GetCommConfig(hComm,&cfg,&cfgSize);
	int bps = baud;
	sprintf(buf,"baud=%d parity=N data=8 stop=1",bps);

	#if (_MSC_VER)       // microsoft visual studio
		// msvc doesn't like BuildCommDCB,
		//so we need to use this version: BuildCommDCBA
		if(!BuildCommDCBA(buf,&cfg.dcb)){
			ofLog(OF_LOG_ERROR,"ofxSerial: unable to build comm dcb; (%s)",buf);
		}
	#else
		if(!BuildCommDCB(buf,&cfg.dcb)){
			ofLog(OF_LOG_ERROR,"ofxSerial: Can't build comm dcb; %s",buf);
		}
	#endif


	// Set baudrate and bits etc.
	// Note that BuildCommDCB() clears XON/XOFF and hardware control by default

	if(!SetCommState(hComm,&cfg.dcb)){
		ofLog(OF_LOG_ERROR,"ofxSerial: Can't set comm state");
	}
	//ofLog(OF_LOG_NOTICE,buf,"bps=%d, xio=%d/%d",cfg.dcb.BaudRate,cfg.dcb.fOutX,cfg.dcb.fInX);

	// Set communication timeouts (NT)
	COMMTIMEOUTS tOut;
	GetCommTimeouts(hComm,&oldTimeout);
	tOut = oldTimeout;
	// Make timeout so that:
	// - return immediately with buffered characters
	tOut.ReadIntervalTimeout=MAXDWORD;
	tOut.ReadTotalTimeoutMultiplier=0;
	tOut.ReadTotalTimeoutConstant=0;
	SetCommTimeouts(hComm,&tOut);

	bInited = true;
	return true;
	//---------------------------------------------
	#endif
	//---------------------------------------------
}


//----------------------------------------------------------------
int ofxSerial::writeBytes(unsigned char * buffer, int length){

	if (!bInited){
		ofLog(OF_LOG_ERROR,"ofxSerial: serial not inited");
		return OF_SERIAL_ERROR;
	}

	//---------------------------------------------
	#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	    int numWritten = write(fd, buffer, length);
		if(numWritten <= 0){
			if ( errno == EAGAIN )
				return 0;
			ofLog(OF_LOG_ERROR,"ofxSerial: Can't write to com port, errno %i (%s)", errno, strerror(errno));
			return OF_SERIAL_ERROR;
		}

		ofLog(OF_LOG_VERBOSE,"ofxSerial: numWritten %i", numWritten);

	    return numWritten;
    #endif
    //---------------------------------------------

    //---------------------------------------------
	#ifdef TARGET_WIN32
		DWORD written;
		if(!WriteFile(hComm, buffer, length, &written,0)){
			 ofLog(OF_LOG_ERROR,"ofxSerial: Can't write to com port");
			 return OF_SERIAL_ERROR;
		}
		ofLog(OF_LOG_VERBOSE,"ofxSerial: numWritten %i", (int)written);
		return (int)written;
	#else
		return 0;
	#endif
	//---------------------------------------------

}

//----------------------------------------------------------------
int ofxSerial::readBytes(unsigned char * buffer, int length){

	if (!bInited){
		ofLog(OF_LOG_ERROR,"ofxSerial: serial not inited");
		return OF_SERIAL_ERROR;
	}

	//---------------------------------------------
	#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
		int nRead = read(fd, buffer, length);
		if(nRead < 0){
			if ( errno == EAGAIN )
				return OF_SERIAL_NO_DATA;
			ofLog(OF_LOG_ERROR,"ofxSerial: trouble reading from port, errno %i (%s)", errno, strerror(errno));
			return OF_SERIAL_ERROR;
		}
		return nRead;
    #endif
    //---------------------------------------------

    //---------------------------------------------
	#ifdef TARGET_WIN32
		DWORD nRead = 0;
		if (!ReadFile(hComm,buffer,length,&nRead,0)){
			ofLog(OF_LOG_ERROR,"ofxSerial: trouble reading from port");
			return OF_SERIAL_ERROR;
		}
		return (int)nRead;
	#endif
	//---------------------------------------------
}

//----------------------------------------------------------------
bool ofxSerial::writeByte(unsigned char singleByte){

	if (!bInited){
		ofLog(OF_LOG_ERROR,"ofxSerial: serial not inited");
		//return OF_SERIAL_ERROR; // this looks wrong.
		return false;
	}

	unsigned char tmpByte[1];
	tmpByte[0] = singleByte;

	//---------------------------------------------
	#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	    int numWritten = 0;
	    numWritten = write(fd, tmpByte, 1);
		if(numWritten <= 0 ){
			if ( errno == EAGAIN )
				return 0;
			 ofLog(OF_LOG_ERROR,"ofxSerial: Can't write to com port, errno %i (%s)", errno, strerror(errno));
			 return OF_SERIAL_ERROR;
		}
		ofLog(OF_LOG_VERBOSE,"ofxSerial: written byte");


		return (numWritten > 0 ? true : false);
    #endif
    //---------------------------------------------

    //---------------------------------------------
	#ifdef TARGET_WIN32
		DWORD written = 0;
		if(!WriteFile(hComm, tmpByte, 1, &written,0)){
			 ofLog(OF_LOG_ERROR,"ofxSerial: Can't write to com port");
			 return OF_SERIAL_ERROR;;
		}

		ofLog(OF_LOG_VERBOSE,"ofxSerial: written byte");

		return ((int)written > 0 ? true : false);
	#endif
	//---------------------------------------------

}

//----------------------------------------------------------------
int ofxSerial::readByte(){

	if (!bInited){
		ofLog(OF_LOG_ERROR,"ofxSerial: serial not inited");
		return OF_SERIAL_ERROR;
	}

	unsigned char tmpByte[1];
	memset(tmpByte, 0, 1);

	//---------------------------------------------
	#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
		int nRead = read(fd, tmpByte, 1);
		if(nRead < 0){
			if ( errno == EAGAIN )
				return OF_SERIAL_NO_DATA;
			ofLog(OF_LOG_ERROR,"ofxSerial: trouble reading from port, errno %i (%s)", errno, strerror(errno));
            return OF_SERIAL_ERROR;
		}
		if(nRead == 0)
			return OF_SERIAL_NO_DATA;
    #endif
    //---------------------------------------------

    //---------------------------------------------
	#ifdef TARGET_WIN32
		DWORD nRead;
		if (!ReadFile(hComm, tmpByte, 1, &nRead, 0)){
			ofLog(OF_LOG_ERROR,"ofxSerial: trouble reading from port");
			return OF_SERIAL_ERROR;
		}
	#endif
	//---------------------------------------------

	return (int)(tmpByte[0]);
}


//----------------------------------------------------------------
void ofxSerial::flush(bool flushIn, bool flushOut){

	if (!bInited){
		ofLog(OF_LOG_ERROR,"ofxSerial: serial not inited");
		return;
	}

	int flushType = 0;

	//---------------------------------------------
	#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
		if( flushIn && flushOut) flushType = TCIOFLUSH;
		else if(flushIn) flushType = TCIFLUSH;
		else if(flushOut) flushType = TCOFLUSH;
		else return;

		tcflush(fd, flushType);
    #endif
    //---------------------------------------------

    //---------------------------------------------
	#ifdef TARGET_WIN32
		if( flushIn && flushOut) flushType = PURGE_TXCLEAR | PURGE_RXCLEAR;
		else if(flushIn) flushType = PURGE_RXCLEAR;
		else if(flushOut) flushType = PURGE_TXCLEAR;
		else return;

		PurgeComm(hComm, flushType);
	#endif
	//---------------------------------------------

}

void ofxSerial::drain(){
    if (!bInited){
	ofLog(OF_LOG_ERROR,"ofxSerial: serial not inited");
	return;
    }

    #if defined( TARGET_OSX ) || defined( TARGET_LINUX )
        tcdrain( fd );
    #endif
}

//-------------------------------------------------------------
int ofxSerial::available(){

	if (!bInited){
		ofLog(OF_LOG_ERROR,"ofxSerial: serial not inited");
		return OF_SERIAL_ERROR;
	}

	int numBytes = 0;

	//---------------------------------------------
	#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
		ioctl(fd,FIONREAD,&numBytes);
	#endif
    //---------------------------------------------

    //---------------------------------------------
	#ifdef TARGET_WIN32
	COMSTAT stat;
       	DWORD err;
       	if(hComm!=INVALID_HANDLE_VALUE){
           if(!ClearCommError(hComm, &err, &stat)){
               numBytes = 0;
           } else {
               numBytes = stat.cbInQue;
           }
       	} else {
           numBytes = 0;
       	}
	#endif
    //---------------------------------------------

	return numBytes;
}


void ofxSerial::writeLine(string str) {
    str+="\n";
    writeBytes((unsigned char*)str.c_str(), str.size());
}

string ofxSerial::readLine(char until) {
    //static string str; //static is cross-instance so not safe? right?
    stringstream ss;
    char ch;
    int ttl=1000;
    while ((ch=readByte())>0 && ttl-->0 && ch!=until) {
        if (ch==OF_SERIAL_ERROR) cout << "OF_SERIAL_ERROR" << endl;
        ss << ch;
    }
    lineBuffer+=ss.str();
    if (ch==until) {
        string tmp=lineBuffer;
        lineBuffer="";
        return tmp; //ofxTrimString(tmp);
    } else {
        return "";
    }
}

