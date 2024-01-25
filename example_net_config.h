// Enter your WiFi ssid and password for network A when available

char * ssid = "***********";
char * password = "***********";

// Retries the above password a number of times before trying the second password
// If second password also fails after retries starts again with the first
// IF first succeeds A is shown on top left of display else B
#define RETRY_PASSWORD 4;

// Enter your WiFi ssid and password for alternative B network  

char * ssid2 = "***********";
char * password2 = "***********";


int broadcastPort = 8005;
int listenPort = 8006;