/*Autor: Lucas Eduardo Dias de Oliveira - SENAI Campos Novos - SC

  Caro programador, quando eu escrevi este código, apenas eu e Deus sabíamos como ele
  deveria funcionar, hoje em dia apenas Deus sabe. Se for necessário fazer alguma alteração
  (que sem dúvidas é necessário), busque ler ele atentamente, toda a documentação e lógica
  está descrita abaixo, além disso, adicione suas horas de trabalho, somente como medida
  administrativa para caso algum dia precise.

  Total de horas gastas: 50h

  Seu princípio de funcionamento é relativamente simples, apenas busca controlar um braço robótico
  que contém as seguintes características:
    > 4 graus de liberdade
    > pinça con contato angular
    > composto por 5 servos motores com rotação de 180°
  
  Além disso, ele atua junto com uma esteira, que seu acionamento é feito através de uma ponte H
  L298 (compacta) que por sua vez, aciona um motor DC de 5 a 12V, vale ressaltar que o próprio
  motor já possui sistema de redução de velocidade de forma mecânica, não sendo necessário grandes 
  variações de velocidade feitas através de PWM.
  A esteira também contará com sensores para a identificação da posição das peças, medidas, ou não
  conformidades.

  Para controlar o braço robótico, ele conta com três modos de operação:
    > Modo manual: a movimentação é feita com a comunicação bluetooth;
    > Modo de programação: a movimentação é feita com a comunicação bluetooth, porém seus movimentos
    são gravados na EEPROM do próprio microcontrolador;
    > Modo automático: a movimentação é feita conforme a programação do "modo de programação".

  A integração do braço robótico é feita com um display touch screen, neste caso da Nextion, funcionando
  como uma espécie de IHM (Interface Homem Máquina), este deverá fazer a seleção do modo de operação,
  auxiliar na programação do braço, mostrar o estado do funcionamento, informações do processo etc...

  //Velocidades de transmissão de bits: 9600; 19200; 38400; 57600; 115200

  >A POSIÇÃO GRAVADA NA EEPROM PARA REFERENCIAR  O TERMINO DA ROTINA AUTOMATICA
  SERÁ A POSIÇÃO 1023
*/

//Bibliotecas
#include <Arduino.h>          //Biblioteca arduino
#include <SoftwareSerial.h>   //Software serial (módulo bluetooth)
#include <Servo.h>            //Controle dos servos motores
//#include <tcs3200.h>          //Sensor de cor
#include <Ultrasonic.h>       //Sensor ultrassonico
#include <EEPROM.h>           //EEPROM Arduino

//Pinagem

  //Display Nextion
    //Será conectado a serial nativa do arduino

  //Módulo bluetooth
  SoftwareSerial bluetooth (14, 15); //Rx Tx D14=A0 e D15=A1
  byte bufferBluetooth();

  //Sensor ultrassonico


  //Servo motores
  Servo servoGarra;        //Servo da garra
  Servo servoMovGarra;     //Servo que movimenta a garra
  Servo servoVincExt;      //Servo que movimenta o vinculo central
  Servo servoVincCentral;  //Servo que movimenta o vinculo central
  Servo servoBase;         //Servo que gira a base

  #define costTestServo 1  //Movimenta os servo de 0° a 180°
  byte tratamentoControleBluetooth(byte returnBluetoothChar); // Altera as variaveis de controle de angulo
  void atualizaAngServo();
  byte angServo[5]={90, 90, 90, 90, 90};
  byte servoControlado=0;
  #define angIncrementoServo 5
  #define angDecrementoServo 5


  //Esteira
  void controleEsteira();
  boolean estadoEsteira = 0;
  boolean velocidadeEsteira = 1;
  boolean sentidoEsteira = 0;
  #define pinVelEst 3
  #define pinEstadoEst 2
  #define velocidadeUmEsteira 100
  #define velocidadeDoisEsteira 255

  //Controle automático braço robótico
  #define linhasProgram 100
  void gravarAngServos();
  signed int linhaProgramada = 0;
  void controleAutomatico();
  #define tempoControleAutomatico 2000
  boolean modoDeControleBr = 0;
  void atualizaDadosControlBraco();
  byte linhaExecutada = 0;


  //Display nextion
  SoftwareSerial nextion (16,17); //16 17

  byte returnBufferNextion();

  void atualizaTxtNextion(String objName, String value);
  void tratatamentoReturnNextion();
  void atualizaNextion();
  void atualizaDadosEsteira();
  void programPageBraco ();

  int numeroPcs = 0;
  int horasTrabalhadas = 0;

  //Temporizador
  unsigned long int tempoRefMillis = 0;

  //Sistema
  byte modoControleSystem = 0;
  byte comandosDelay []={ 54,55,56,57,71,73};


void setup(){

  //Serial
    Serial.begin(115200);
    delay(500);

  //Serial bluetooth 
    bluetooth.begin(115200);
    delay(500);

  //Serial display
    nextion.begin(115200);
    delay(500);

  //Pinos da esteira
    pinMode(pinEstadoEst, OUTPUT);
    pinMode(pinVelEst, OUTPUT);
    digitalWrite(pinEstadoEst, LOW);
    digitalWrite(pinVelEst, LOW);

  //Para testar servos motores mude test=1
    servoGarra.attach(5);
    servoMovGarra.attach(6);
    servoVincExt.attach(9);
    servoVincCentral.attach(10);
    servoBase.attach(11);
    //Teste servos
    boolean testServo=costTestServo; //1 para testar servos
    if(testServo==1){
      servoGarra.write(0);
      servoMovGarra.write(0);
      servoVincExt.write(0);
      servoVincCentral.write(0);
      servoBase.write(0);
      delay(2000);
      servoGarra.write(90);
      servoMovGarra.write(90);
      servoVincExt.write(90);
      servoVincCentral.write(90);
      servoBase.write(90);
      delay(2000);
      servoGarra.write(180);
      servoMovGarra.write(180);
      servoVincExt.write(180);
      servoVincCentral.write(180);
      servoBase.write(180);
      delay(2000);
    }

    //Movimenta zero máquina
    servoGarra.write(90);
    servoMovGarra.write(90);
    servoVincExt.write(90);
    servoVincCentral.write(90);
    servoBase.write(90);
    delay(2000);

    //Limpeza EEPROM do Arduino
    /*for(int eeprom=0; eeprom<1023; eeprom++){
      EEPROM.write(eeprom, 0);
      Serial.println("Pos :" + String(eeprom));
      Serial.print(" value: " + String(EEPROM.read(eeprom)));
      Serial.println(EEPROM.read(eeprom));
    }*/

    //Carrega posições para os servos
    
}

void loop(){
  //programPageBraco();
  tratatamentoReturnNextion();


}

//Função para ler valores que chegam na buffer do bluetooth
byte bufferBluetooth(){
  bluetooth.listen();
  delay(50);
  if(bluetooth.available()!=0){
    byte bufferSerialBluetooth[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for(byte x=0; x<sizeof(bufferSerialBluetooth)/sizeof(bufferSerialBluetooth[0]);x++){
      bufferSerialBluetooth[x]=bluetooth.read();
      //Serial.println(bufferSerialBluetooth[x]);
            
    }
  for(byte y=0; y<sizeof(comandosDelay)/sizeof(comandosDelay[0]);y++){
        if(comandosDelay[y]==bufferSerialBluetooth[0]){
          delay(100);
          while(bluetooth.available()!=0){
            bluetooth.read();
          }
        }
      }
  return bufferSerialBluetooth[0];
  } else {
    return 0;
  } 
}

//A função "tratamentoControleBluetooth", toma as decisões conforme o que é 
//lido na buffer serial do módulo bluetooth
byte tratamentoControleBluetooth(byte returnBluetoothChar){
  if(returnBluetoothChar!=0){
  switch (returnBluetoothChar)
    {
    //Altera servo que está sendo controlado:
      case 54:
          servoControlado++;
          if(servoControlado>=5){
            servoControlado=0;
          }
          while(bluetooth.available()!=0){
              bluetooth.read();
          }
          delay(250);
          atualizaTxtNextion("tServoControl", String(servoControlado));
          atualizaTxtNextion("tAngServo", String(angServo[servoControlado]));
        break;
      
    //Gravar posição servo motor
      case 55:
          gravarAngServos();
        break;

    //Acionar e desacionar a esteira
      case 56:
          estadoEsteira=!estadoEsteira;
        break;
    
    //Sair do modo de controle via bluetooth
      case 57:
          if(linhaProgramada!=0){
            EEPROM.update(1023, linhaProgramada);
            linhaProgramada=0;
            modoDeControleBr=1;
          }
          
          nextion.listen();
          nextion.print("page page3");
          controleAutomatico();

        break;

    //Aumenta ou diminui angulo do servo que está sendo controlado
      case 70:
        angServo[servoControlado]=angServo[servoControlado]+angIncrementoServo;
        if(angServo[servoControlado]>=180){
          angServo[servoControlado]=180;
        }
        atualizaAngServo();
        atualizaTxtNextion("tAngServo", String(angServo[servoControlado]));
        break;

      case 72:
        angServo[servoControlado]=angServo[servoControlado]-angDecrementoServo;
        if(angServo[servoControlado]<=10){
          angServo[servoControlado]=10;
        }
        atualizaAngServo();
        atualizaTxtNextion("tAngServo", String(angServo[servoControlado]));
        break;

    //Avança linha de programação
      case 71:
          linhaProgramada++;
          if(linhaProgramada>=linhasProgram){
            linhaProgramada=linhasProgram;
          }
          atualizaTxtNextion("tProgramLine", String(linhaProgramada));
          atualizaTxtNextion("tAngServo", String(angServo[servoControlado]));
        break;

    //Retorna linha de programação
      case 73:
          linhaProgramada--;
          if(linhaProgramada<=0){
            linhaProgramada=0;
          }
          atualizaTxtNextion("tProgramLine", String(linhaProgramada));
          atualizaTxtNextion("tAngServo", String(angServo[servoControlado]));
        break;   

    default:
      break;
    }
    return 255;
  }        
    //controleEsteira();
    return 0;
  }


//Atualiza o angulo dos servos motores do braço
void atualizaAngServo(){
  servoGarra.write(map(angServo[0],10,180,0,180));
  servoMovGarra.write(map(angServo[1],10,180,0,180));
  servoVincExt.write(map(angServo[2],10,180,0,180));
  servoVincCentral.write(map(angServo[3],10,180,0,180));
  servoBase.write(map(angServo[4],10,180,0,180));
}

//Controla a esteira
void controleEsteira(){
  //digitalWrite(pinEstadoEst, sentidoEsteira);
  if(estadoEsteira==HIGH){

    if(sentidoEsteira==0){
      if (velocidadeEsteira==0) {
        digitalWrite(pinEstadoEst, LOW);
        analogWrite(pinVelEst, velocidadeUmEsteira);
      }else{
        digitalWrite(pinEstadoEst, LOW);
        analogWrite(pinVelEst, velocidadeDoisEsteira);
      }
    }

    if(sentidoEsteira==1){
      if (velocidadeEsteira==0){
        digitalWrite(pinEstadoEst, HIGH);
        analogWrite(pinVelEst, map(velocidadeUmEsteira,0,255,255,0));
      }else{
        digitalWrite(pinEstadoEst, HIGH);
        analogWrite(pinVelEst, map(velocidadeDoisEsteira,0,255,255,0));
      }
      }
    } else {
      digitalWrite(pinEstadoEst, LOW);
      digitalWrite(pinVelEst, LOW);
    }

}

//Entra no modo de gravação
void gravarAngServos(){
  EEPROM.update(servoControlado+linhaProgramada*10,angServo[servoControlado]);
  Serial.println("Angulo salvo:"+ String(EEPROM.read(servoControlado+linhaProgramada*10)));
  linhaExecutada=0;
}

//Acionamento automático do braço
void controleAutomatico(){
  controleEsteira();
  nextion.listen();
  while(modoDeControleBr==1){
    servoGarra.write        (EEPROM.read(0+(linhaExecutada*10)));
    servoMovGarra.write     (EEPROM.read(1+(linhaExecutada*10)));
    servoVincExt.write      (EEPROM.read(2+(linhaExecutada*10)));
    servoVincCentral.write  (EEPROM.read(3+(linhaExecutada*10)));
    servoBase.write         (EEPROM.read(4+(linhaExecutada*10)));

    if(millis()>=tempoRefMillis){
      tempoRefMillis=millis()+tempoControleAutomatico;
      linhaExecutada++;
    }
    if(linhaExecutada==EEPROM.read(1023)){
      linhaExecutada=0;
    }

    if(nextion.available()!=0){
      tratatamentoReturnNextion();
    } 
    
    
  }
    
}

//Retorna valor da buffer do nextion
byte returnBufferNextion(){
  nextion.listen();
  delay(50);
  byte bufferNextion [16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  if(nextion.available()!=0 and nextion.isListening()){

    for(byte x=0; x<sizeof(bufferNextion)/sizeof(bufferNextion[0]);x++){
      if(nextion.available()!=0){
        bufferNextion[x]=nextion.read();
      }else{
        break;
      }    
    }

    for(byte x=0; x<sizeof(bufferNextion)/sizeof(bufferNextion[0]);x++){
      if(bufferNextion[x]!= 4 and bufferNextion[x] !=255 and bufferNextion[x] !=0){
        return bufferNextion[x];
      }
    }
  }
    return 255;
}

//Atuliza dados dos objetos nextion
void atualizaTxtNextion(String objName, String value){
    nextion.print(objName + ".txt=\"");
    nextion.print(value);
    nextion.print("\"");
    nextion.write(0xff);
    nextion.write(0xff);
    nextion.write(0xff);

}

//Toma decisão conforme retorno da buffer do nextion
void tratatamentoReturnNextion(){
  byte returnNextion = returnBufferNextion();
  if(returnNextion!=255){
    Serial.write(returnNextion);
    Serial.print("\n");

    switch (returnNextion){

      //Atualiza página nextion
      case 120:
          atualizaNextion();
        break;

      //Atualiza dados da esteira, além de ligar e desligar
      case 65:
          estadoEsteira=!estadoEsteira;
          controleEsteira();
          atualizaDadosEsteira();
        break;

      //Controle automático ou manual do braço
      case 66:
          modoDeControleBr=!modoDeControleBr;
          atualizaDadosControlBraco();
        break;

      //Entra no modo de gravação
      case 122:
        byte charRetornoBluetooth = bufferBluetooth();
        atualizaDadosControlBraco();
        while(charRetornoBluetooth!=57){
          charRetornoBluetooth = bufferBluetooth();
          if(charRetornoBluetooth!=0){
            tratamentoControleBluetooth(charRetornoBluetooth);
            atualizaDadosControlBraco();
          }          
        }
        break;




    }
  }
  
}

//Conjunto de atualizações nextion
void atualizaNextion(){
  atualizaDadosEsteira();
  atualizaDadosControlBraco();
}

//Atualiza dados específicos da esteira no nextion
void atualizaDadosEsteira(){
  if(estadoEsteira==HIGH){
        atualizaTxtNextion("tStateEst", "ON");
      }else{
        atualizaTxtNextion("tStateEst", "OFF");
      }

  if(velocidadeEsteira==HIGH){
        atualizaTxtNextion("txtVelEst", "1");
      }else{
        atualizaTxtNextion("txtVelEst", "0");
      }

  atualizaTxtNextion("txtNP", String(numeroPcs));
  atualizaTxtNextion("txtHrTr", String(horasTrabalhadas));
  
}

//Atualiza dados específicos do braço no nextion
void atualizaDadosControlBraco(){
  if(modoDeControleBr==1){
    atualizaTxtNextion("tStateBr", "AUTO");
  }else{
    atualizaTxtNextion("tStateBr", "MANUAL");
  }

  atualizaTxtNextion("tProgramLine", String(linhaProgramada));
  atualizaTxtNextion("tAngServo", String(angServo[servoControlado]));
  atualizaTxtNextion("tServoControl", String(servoControlado));
  
}

void programPageBraco (){
  nextion.print("page page4");
  nextion.write(0xff);
  nextion.write(0xff);
  nextion.write(0xff);
}

