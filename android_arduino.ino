/*
Arduino communicates with both the W5100 and SD card using the SPI bus (through the 
ICSP header). This is on digital pins 10, 11, 12, and 13 on the Uno and pins 50, 51, 
and 52 on the Mega. On both boards, pin 10 is used to select the W5100 and pin 4 for 
the SD card. These pins cannot be used for general I/O. On the Mega, the hardware SS 
pin, 53, is not used to select either the W5100 or the SD card, but it must be kept 
as an output or the SPI interface won't work.

Note that because the W5100 and SD card share the SPI bus, only one can be active at 
a time. If you are using both peripherals in your program, this should be taken care 
of by the corresponding libraries. If you're not using one of the peripherals in your 
program, however, you'll need to explicitly deselect it. To do this with the SD card, 
set pin 4 as an output and write a high to it. For the W5100, set digital pin 10 as a
high output.
*/

#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192,168,1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

const byte numClientes = 3;
// telnet defaults to port 23
EthernetServer server(23);
EthernetClient client;
EthernetClient clients[numClientes];

// Pins configuration
const byte pinLedVerde = 5;
const byte pinBotaoVerde = 8;

const byte pinLedAmarelo = 6;
const byte pinBotaoAmarelo = 7;

const byte pinLedVermelho = 9;
const byte pinBotaoVermelho = 3;

const byte bitPos[] = {0x01, 0x02, 0x04};

byte estadoAtual;
byte estado;
bool valor;

long lastDebounceTime;  // the last time the output pin was toggled
const long debounceDelay = 100;    // the debounce time; increase if the output flickers
String leitura;

void setup() {
  // make the pins outputs:
  pinMode(pinLedVerde, OUTPUT);
  pinMode(pinLedAmarelo, OUTPUT);
  pinMode(pinLedVermelho, OUTPUT);
  
  // make the pins inputs:
  pinMode(pinBotaoVerde, INPUT_PULLUP);
  pinMode(pinBotaoAmarelo, INPUT_PULLUP);
  pinMode(pinBotaoVermelho, INPUT_PULLUP);

  valor = 0;

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  // this check is only needed on the Leonardo:
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // start the Ethernet connection:
  Serial.println("Trying to get an IP address using DHCP");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // initialize the Ethernet device not using DHCP:
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  ip = Ethernet.localIP();
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(ip[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
  // start listening for clients
  server.begin();

  lastDebounceTime = millis();
  estado = 0;
  estadoAtual = 0;

  leitura = "";
}

String ImprimirValor(byte valor)
{
  byte teste;
  String texto;

  texto = "";
  if (valor > 100)
  {
    teste =  valor / 100;
    valor = valor % 100;
    texto.concat(teste);
  }

  if (valor > 10)
  {
    teste =  valor / 10;
    valor = valor % 10;
    texto.concat(teste);
  }

  texto.concat(valor);  
  return texto;
}

void AtualizarValorTCP(byte led, byte valor)
{
  for (byte i = 0; i < numClientes; i++) {
    if (clients[i]) {
      // clear out the input buffer:
      clients[i].flush();
      
      clients[i].print("{\"bt");
      clients[i].print(led);
      clients[i].print("\":");
      clients[i].print(ImprimirValor(valor));
      clients[i].println("}");
    }
  }
}

void Debounce()
{
  valor = digitalRead(pinBotaoVerde);
  delay(1);

  estado = valor;
  estado <<= 1;
  
  valor = digitalRead(pinBotaoAmarelo);
  delay(1);

  estado |= valor;
  estado <<= 1;
  
  valor = digitalRead(pinBotaoVermelho);
  delay(1);

  estado |= valor;

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (estado != estadoAtual) {  
      for (byte i = 0; i < sizeof(bitPos); i++)
      {
        if ((estado & bitPos[i]) != (estadoAtual & bitPos[i]))
        {
          valor = !(estado & bitPos[i]); 
          if (valor > 0)
          {
            valor = 255;
          }
          i++;
          switch(i)
          {
            case 1:            
                digitalWrite(pinLedVerde, valor);                
                Serial.print("Verde: ");
            break;
  
            case 2:
              digitalWrite(pinLedAmarelo, valor);
              Serial.print("Amarelo: ");
            break;
  
            case 3:
              digitalWrite(pinLedVermelho, valor);
              Serial.print("Vermelho: ");
            break;
          }
          Serial.println(valor); // somente para debug
          AtualizarValorTCP(i, valor);
        }
      }

      estadoAtual = estado;
    }
    
    lastDebounceTime = millis();
  }
}

void Parser(char thisChar)
{
  if (thisChar == '\n')
  {
    //{"led1":0}
    
    if (leitura.startsWith("{\"led"))
    {
      byte colon1 = leitura.indexOf("d");
      byte colon2 = leitura.indexOf(":");
      byte btnVal = leitura.substring(colon1 + 1, colon2).toInt();
      byte ledVal = leitura.substring(colon2 + 1).toInt();

      switch (btnVal)
      {
        case 1:
          analogWrite(pinLedVerde, ledVal);
          ImprimirValor(ledVal);
        break;

        case 2:
          analogWrite(pinLedAmarelo, ledVal);
          ImprimirValor(ledVal);
        break;

        case 3:
          analogWrite(pinLedVermelho, ledVal);
          ImprimirValor(ledVal);
        break;

        default:
          Serial.write("Opção inválida\n");
      }
    }
     
    leitura = "";
  }
  else{
    leitura.concat(thisChar);
  }
}

void loop() {  
  Debounce();
  
  // wait for a new client:
  client = server.available();

  // when the client sends the first byte, say hello:
  if (client) {
    boolean newClient = true;
    for (byte i = 0; i < numClientes; i++) {
      //check whether this client refers to the same socket as one of the existing instances:
      if (clients[i] == client) {
        newClient = false;
        break;
      }
    }

    if (newClient) {
      //check which of the existing clients can be overridden:
      for (byte i = 0; i < numClientes; i++) {
        if (!clients[i] && clients[i] != client) {
          clients[i] = client;
          // clear out the input buffer:
          client.flush();
          Serial.println("We have a new client: ");
          Serial.println(i);
          client.print("Hello, client number: ");
          client.println(i);
          break;
        }
      }
    }

    if (client.available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = client.read();
      // echo the bytes back to all other connected clients:
      for (byte i = 0; i < numClientes; i++) {
        if (clients[i] && (clients[i] != client)) {
          clients[i].write(thisChar);
        }
      }
      // echo the bytes to the server as well:
      Serial.write(thisChar);

      Parser(thisChar);
    }
  }
  
  for (byte i = 0; i < numClientes; i++) {
    if (!(clients[i].connected())) {
      // client.stop() invalidates the internal socket-descriptor, so next use of == will allways return false;
      clients[i].stop();
    }
  }
}
