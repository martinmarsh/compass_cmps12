// Enter your WiFi ssid and password for network A when available

const char * ssid = "***********";
const char * password = "***********";

// Retries the above password a number of times before trying the second password
// If second password also fails after retries starts again with the first
// IF first succeeds A is shown on top left of display else B
#define RETRY_PASSWORD 4;

// Enter your WiFi ssid and password for alternative B network  

const char * ssid2 = "***********";
const char * password2 = "***********";


const int broadcastPort = 8005;
const int listenPort = 8006;