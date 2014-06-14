#include <iostream>
#include  <iomanip>
#include <libusb.h>

#define GIMIC_USBVID 0x16c0
#define GIMIC_USBPID 0x05e5

using namespace std;

void printdev(libusb_device *dev); //prototype of the function

int WriteBytes(libusb_device_handle *dev_handle, unsigned char *data, int *bytes)
{
    int r = 0;
    int inBytes = *bytes;
    r = libusb_bulk_transfer(dev_handle, (2 | LIBUSB_ENDPOINT_OUT), data, inBytes, bytes, 0);
    if (r == 0 && *bytes == inBytes)
		cout<<"Writing Successful!"<<endl;
	else
		cout<<"Write Error"<<endl;
    return r;
}

int ReadBytes(libusb_device_handle *dev_handle, unsigned char *data, int *bytes, int timeOut)
{
    int r = 0;
    int reqBytes = *bytes;
    r = libusb_bulk_transfer(dev_handle, (5 | LIBUSB_ENDPOINT_IN), data, reqBytes, bytes, timeOut);
    if (r == 0 && *bytes == reqBytes)
		cout<<"Reading Successful!"<<endl;
	else
		cout<<"Read Error"<<endl;
    return r;
}

void PrintHexData(const unsigned char *data, int bytes)
{
    for (int i=0; i<bytes; i++) {
        cout    << setw( 2 )       // フィールド幅 2
                << setfill( '0' )  // 0で埋める
                << hex             // 16進数
                << uppercase       // 大文字表示
                << ( int )data[i]
                << " ";
        
        if( i % 16 == 15 )  std::cout << std::endl;

    }
}

int main()
{
	libusb_device_handle *dev_handle; //a device handle
	libusb_context *ctx = NULL; //a libusb session
	int r; //for return values
	r = libusb_init(&ctx); //initialize the library for the session we just declared
	if(r < 0) {
		cout<<"Init Error "<<r<<endl; //there was an error
		return 1;
	}
	libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation
    
    
	libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
    ssize_t cnt; //holding number of devices in list
	cnt = libusb_get_device_list(ctx, &devs); //get the list of devices
	if(cnt < 0) {
		cout<<"Get Device Error"<<endl; //there was an error
		return 1;
	}
	cout<<cnt<<" Devices in list."<<endl;
    for (int i=0; i<cnt; i++) {
        printdev(devs[i]);
    }
	libusb_free_device_list(devs, 1); //free the list, unref the devices in it
    
    
    // VendorIDとProductIDを指定してデバイスを開く
	dev_handle = libusb_open_device_with_vid_pid(ctx, GIMIC_USBVID, GIMIC_USBPID); //these are vendorID and productID I found for my usb device
	if(dev_handle == NULL)
		cout<<"Cannot open device"<<endl;
	else
		cout<<"Device Opened"<<endl;
    
    
    // カーネルドライバが有効な場合、無効にして制御を取得する
	if (libusb_kernel_driver_active(dev_handle, 0) == 1) { //find out if kernel driver is attached
		cout<<"Kernel Driver Active"<<endl;
		if(libusb_detach_kernel_driver(dev_handle, 0) == 0) //detach it
			cout<<"Kernel Driver Detached!"<<endl;
	}
	r = libusb_claim_interface(dev_handle, 0); //claim interface 0 (the first) of device (mine had jsut 1)
	if(r < 0) {
		cout<<"Cannot Claim Interface"<<endl;
		return 1;
	}
	cout<<"Claimed Interface"<<endl;
	
    
    // 書き込みと読み込みのテスト
	unsigned char *data = new unsigned char[64]; //data to write
    int transBytes;
    
    // Hard Reset
	data[0] = 0xfd;
    data[1] = 0x81;
    data[2] = 0xff;
    transBytes = 3;
    r = WriteBytes(dev_handle, data, &transBytes);

    usleep(1000);

	data[0] = 0xfd;
    data[1] = 0xb0;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0xff;
    transBytes = 5;
    r = WriteBytes(dev_handle, data, &transBytes);
    transBytes = 64;
    r = ReadBytes(dev_handle, data, &transBytes, 10);
    PrintHexData(data, transBytes);

    
    data[0] = 0x00;
    data[1] = 0xcc;
    data[2] = 0xff;
    transBytes = 3;
    r = WriteBytes(dev_handle, data, &transBytes);

    usleep(50000);
 
	data[0] = 0xfd;
    data[1] = 0xb0;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0xff;
    transBytes = 5;
    r = WriteBytes(dev_handle, data, &transBytes);
    transBytes = 64;
    r = ReadBytes(dev_handle, data, &transBytes, 10);
    PrintHexData(data, transBytes);

    // 解放処理
    r = libusb_release_interface(dev_handle, 0);
	if(r!=0) {
		cout<<"Cannot Release Interface"<<endl;
		return 1;
	}
	cout<<"Released Interface"<<endl;
    
	libusb_close(dev_handle); //close the device we opened
	libusb_exit(ctx); //needs to be called to end the
    
	delete[] data; //delete the allocated memory for data
	return 0;
}

void printdev(libusb_device *dev)
{
	libusb_device_descriptor desc;
	int r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
		cout<<"failed to get device descriptor"<<endl;
		return;
	}
    if (desc.idVendor == GIMIC_USBVID && desc.idProduct == GIMIC_USBPID) {
        cout<<"Number of possible configurations: "<<(int)desc.bNumConfigurations<<"  ";
        cout<<"Device Class: "<<(int)desc.bDeviceClass<<"  ";
        cout<<"VendorID: "<<desc.idVendor<<"  ";
        cout<<"ProductID: "<<desc.idProduct<<endl;
        libusb_config_descriptor *config;
        libusb_get_config_descriptor(dev, 0, &config);
        cout<<"Interfaces: "<<(int)config->bNumInterfaces<<" ||| ";
        const libusb_interface *inter;
        const libusb_interface_descriptor *interdesc;
        const libusb_endpoint_descriptor *epdesc;
        for(int i=0; i<(int)config->bNumInterfaces; i++) {
            inter = &config->interface[i];
            cout<<"Number of alternate settings: "<<inter->num_altsetting<<" | ";
            for(int j=0; j<inter->num_altsetting; j++) {
                interdesc = &inter->altsetting[j];
                cout<<"Interface Number: "<<(int)interdesc->bInterfaceNumber<<" | ";
                cout<<"Number of endpoints: "<<(int)interdesc->bNumEndpoints<<" | ";
                for(int k=0; k<(int)interdesc->bNumEndpoints; k++) {
                    epdesc = &interdesc->endpoint[k];
                    cout<<"Descriptor Type: "<<(int)epdesc->bDescriptorType<<" | ";
                    cout<<"EP Address: "<<(int)epdesc->bEndpointAddress<<" | ";
                }
            }
        }
        cout<<endl<<endl<<endl;
        libusb_free_config_descriptor(config);
    }
}
