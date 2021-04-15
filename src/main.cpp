#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>


//Mapeamento das entradas/Saidas ESP01 (ESP8266)
#define GPIO0_SpiCs2           0
#define GPIO1_U0TXD_SpiCs1     1
#define GPIO2_U1TXD            2
#define GPIO3_U0RXD            3


#define TempoDePortalAP 180
#define TempoParaTentarConectar 30
//#define TempoParaTentarConectar 2 // TESTES


//______________________VARIAVEIS GLOBAIS____________________________
String header; // Variável para armazenar a solicitação HTTP

// Variaveis para armazenar o estado atual das saidas
String EstadoGPIO0 = "DESLIGADO";
String EstadoGPIO2 = "DESLIGADO";

// Variaveis para receber as credencias de WiFi salvas na memoria
String SSIDStation;
String PassStation;

// Variaveis para receber as credencias do dispositivo quando ele operar em modo Access Point
const char apName[10] = "MEU ESP01"; //SSID
char const apPassword[20] = "PABLLOLINS"; //SENHA

bool shouldSaveConfig = false; // sinalizar para salvar dados
//___________________________________________________________________


// retorno de função notificando a necessidade de salvar as configurações
void saveConfigCallback () {  
  shouldSaveConfig = true;
}


//Configura o servidor WEB na porta 80
WiFiServer server(80);



//_________________________________________________________________________________________________________________________
void setup() {
  delay(50);
  Serial.begin(9600);
  delay(50);

  pinMode(GPIO2_U1TXD, OUTPUT);
  pinMode(GPIO0_SpiCs2, OUTPUT);

  digitalWrite(GPIO2_U1TXD, LOW);
  digitalWrite(GPIO0_SpiCs2, LOW);

  Serial.println();
  Serial.println();
  Serial.print(F("\nRazão do RESET do módulo = "));
  String resetCause = ESP.getResetReason();
  Serial.println(resetCause);


  /*  Inicia o objeto de nome WifiManager, para a classe WiFiManager*/
  WiFiManager MeuObjetoWM;

  //setSaveConfigCallback() --> chamado quando as configurações são alteradas e a conexão é bem-sucedida
  MeuObjetoWM.setSaveConfigCallback(saveConfigCallback);

  //Descomente a linha a baixo para apagar todo o armazenamento de informações
  //MeuObjetoWM.resetSettings();
  
  // Configura IPs estáticos para o portal
  //MeuObjetoWM.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  if((WiFi.SSID() == "") || (WiFi.psk() == "")){ //SE NÂO HOUVER credenciais na memória
    Serial.println("Não há credenciais na memória! "); //informa na serial que não há credenciais
    MeuObjetoWM.setTimeout(TempoDePortalAP); //Configura um tempo para o portal AP
    Serial.print(TempoDePortalAP); //informa na serial o tempo
    Serial.println(" segundos de portal do Soft AP");//informa na serial
    MeuObjetoWM.startConfigPortal(apName, apPassword); //Inicia o portal AP com as credencias informadas neste código
  
  }else{//SE HOUVER credenciais na memória
    Serial.println("Há credenciais na memória! "); //informa na serial   
    
    //define o tempo limite para o dispositivo tentar se conectar a rede gravada em sua memória
    MeuObjetoWM.setConnectTimeout(TempoParaTentarConectar); 

    //define o tempo limite até que o portal de configuração seja desativado, útil para fazer com que tudo tente novamente ou entre no modo de espera em segundos. 
    MeuObjetoWM.setTimeout(TempoDePortalAP); 
    
    //Inicia o método de auto connect, informando os dados de modo Access Point também
    //Busca o SSID e a SENHA da EEPROM e tenta conectar, se não conectar, inicia o modo AP com os dados fornecidos
    if (MeuObjetoWM.autoConnect(apName, apPassword)) { //Caso não consiga conexão com a rede dentro do tempo       
      if(WiFi.status() == WL_CONNECTED){//Se dentro do tempo "TempoParaTentarConectar" conseguir se conectar ao WiFi
        
        Serial.print("Conectado a rede: "); //informa que a conexãofoi bem sucedida
        Serial.println(WiFi.SSID().c_str()); //informa na serial o nome da rede

        Serial.print("Endereço de IP: ");//informa na serial
        Serial.println(WiFi.localIP());//informa na serial o endereço da página do servidor
        delay(100);
      }
      /*else if(WiFi.status() != WL_CONNECTED){
        //Se passar o tempo de buscar rede e não se conectar, apaga todas as informações
        MeuObjetoWM.resetSettings();
        delay(1000);
        Serial.println("PARAMETROS RESETADOS");
        delay(1000);
        ESP.reset();
      } 
      */ 
    }else{            
        Serial.println();

        //define o tempo limite até que o portal de configuração seja desativado, útil para fazer com que tudo tente novamente ou entre no modo de espera em segundos. 
        MeuObjetoWM.setTimeout(TempoDePortalAP); 

        Serial.print((TempoDePortalAP/60));
        Serial.println(" Minutos de portal de configurações AP");

        //Inicia o portal de configuração AP
        //Este parametro da classe também serve para iniciar o portal de configuração, sem tentar se conectar primeiro
        //Se for colocado após a declaração do objeto
        MeuObjetoWM.startConfigPortal(apName, apPassword);

        Serial.println("Não houve ancoragem ao Soft-AP");
        Serial.println();
        Serial.println("ESP entrando em Deep Sleep para economizar bateria");    
        ESP.deepSleep(900e6); //função que inicia o DeepSleep
        //Neste caso 900e6 corresponde a 900000000 microsegundos que é igual a 15 minutos hora        
    }
  }

  server.begin();
}
//_________________________________________________________________________________________________________________________





//_________________________________________________________________________________________________________________________
void loop(){
  WiFiClient client = server.available();   // Vigia a chegada dos clientes no server

  if (client) {                             // Se um cliente se conectar                            
    
    Serial.println("Cliente está conectado");    // Imprime na Serial que o cliente está conectado       
    String currentLine = "";                // cria uma string vazia para receber os dados de entrada do cliente
    while (client.connected()) {            // Permanece nesse laço enquanto houver alguem concetado ao IP
      
      if (client.available()) {             // se houver bytes para ler do cliente

        char c = client.read();             // Declara uma variavel do tipo char, e inicializa ela com o valor lido do cliente
        Serial.write(c);                    // Imprime na serial o que foi lido
        header += c;                        // A string global recebe no final o valor lido 
        if (c == '\n') {                    // Se o char lido for igual a pular uma linha indica que acabou a transmissão

          // Se a linha atual for vazia, você tem dois caracteres de nova lina, esse é o fim da solicitação HTTP do cliente
          if (currentLine.length() == 0) {

            // Os cabeçalhos HTTP sempre começam com um código de resposta (por exemplo, HTTP / 1.1 200 OK)
            // e um tipo de conteúdo para que o cliente saiba o que está por vir, em seguida, uma linha em branco:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Conexão: Fechada");
            client.println();
            
            // Trecho para ligar e desligar GPIOs
            if (header.indexOf("GET /0/on") >= 0) {
              Serial.println("GPIO 0 LIGADO");
              EstadoGPIO0 = "LIGADO";
              digitalWrite(GPIO0_SpiCs2, HIGH);
            } else if (header.indexOf("GET /0/off") >= 0) {
              Serial.println("GPIO 0 DESLIGADO");
              EstadoGPIO0 = "DESLIGADO";
              digitalWrite(GPIO0_SpiCs2, LOW);
            } else if (header.indexOf("GET /2/on") >= 0) {
              Serial.println("GPIO 2 LIGADO");
              EstadoGPIO2 = "LIGADO";
              digitalWrite(GPIO2_U1TXD, HIGH);
            } else if (header.indexOf("GET /2/off") >= 0) {
              Serial.println("GPIO 2 DESLIGADO");
              EstadoGPIO2 = "DESLIGADO";
              digitalWrite(GPIO2_U1TXD, LOW);
            }
            
            // Mostra a página HTML
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            
            // Design CSS para os botoes de ON/OFF 
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Cabeçalho da página
            client.println("<body><h1>MEU ESP-01</h1>");
            
            // Exibe o estado atual LIGADO/DESLIGADO no botão para o GPIO0 
            client.println("<p>RELE DO GPIO 0 ESTA: " + EstadoGPIO0 + "</p>");

            // Se EstadoGPIO0 for desligado ou ligado, altera este valor na pagina web     
            if (EstadoGPIO0=="DESLIGADO") {
              client.println("<p><a href=\"/0/on\"><button class=\"button\">LIGAR</button></a></p>");
            } else {
              client.println("<p><a href=\"/0/off\"><button class=\"button button2\">DESLIGAR</button></a></p>");
            } 
               
            // Exibe o estado atual LIGADO/DESLIGADO no botão para o GPIO2  
            client.println("<p>RELE DO GPIO 2 ESTA: " + EstadoGPIO2 + "</p>");
            
            // Se EstadoGPIO2 for desligado ou ligado, altera este valor na pagina web
            if (EstadoGPIO2=="DESLIGADO") {
              client.println("<p><a href=\"/2/on\"><button class=\"button\">LIGAR</button></a></p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button button2\">DESLIGAR</button></a></p>");
            }
            client.println("</body></html>");
            
            // A resposta HTTP termina com outra linha em branco
            client.println();

            //Finaliza o laço while
            break;

          } else { //se receber uma nova linha, então limpa a variavel de recepção
            currentLine = "";
          }
        } else if (c != '\r') {  //Se o valor lido for qualquer coisa diferete deum caractere de retorno de carro,
          currentLine += c;      // adiciona o caractere lido ao final da string
        }
      }

    }

  
    header = ""; //Esvazia a variavel de recebimento

    client.stop(); //Fecha a conexão
    Serial.println("Cliente desconectado.");
    Serial.println();
  }
}