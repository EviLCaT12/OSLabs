#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <WinSock2.h>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#else
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#endif

using namespace std;
using namespace boost::asio;

string COMM;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <serial_port>" << std::endl;
        return 1;
    }
    COMM = argv[1];
    try {
        io_service io;
        serial_port serial(io);

        // for win "COM1"
        //Linux - "/dev/ttyS0")
        serial.open(COMM);

        if (!serial.is_open()) {
            cerr << "Failed to open serial port" << endl;
            return 1;
        }

#ifdef _WIN32
        // Get serial port handle
        HANDLE serialHandle = serial.native_handle();

        // Configure serial port settings (replace with your desired settings)
        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(serialHandle, &dcbSerialParams)) {
            cerr << "Failed to get serial port state" << endl;
            return 1;
        }

        // Set flow control to none
        dcbSerialParams.fOutxCtsFlow = false;
        dcbSerialParams.fOutxDsrFlow = false;
        dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
        dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;

        if (!SetCommState(serialHandle, &dcbSerialParams)) {
            cerr << "Failed to set serial port state" << endl;
            return 1;
        }
#endif

        while (true) {
            double temperature = rand() % 4000 / 100.0;

            string data = to_string(temperature);
            data.resize(5, '0');
            data += "\n";

            write(serial, buffer(data));
            cerr <<"writed " << data << endl;

            boost::this_thread::sleep_for(boost::chrono::seconds(5));
        }
    } catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}