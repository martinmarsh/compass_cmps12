// Enter your wifi ssid and password

const char * ssid = "***********";
const char * password = "***********";

// Retries the above password a number of times before trying the second password
// If seconnd password also fails after retries starts again with the first
#define RETRY_PASSWORD 4;

const char * ssid2 = "***********";
const char * password2 = "***********";


const int broadcastPort = 8005;
const int listenPort = 8006;