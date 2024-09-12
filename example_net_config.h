// Enter your WiFi ssid and password for network A when available
//Enclose strings in double quotes without anything else on the line

#define SSID_A  "********"
#define PASSWORD_A "***********"

// Retries the above password a number of times before trying the second password
// If second password also fails after retries starts again with the first
// IF first succeeds A is shown on top left of display else B
#define RETRY_PASSWORD 4

// Enter your WiFi ssid and password for alternative B network  

#define SSID_B "********"
#define PASSWORD_B  "*********"


#define BROADCAST_PORT 8008
#define BROADCAST_DIRECT_PORT 8005
#define LISTEN_PORT  8006