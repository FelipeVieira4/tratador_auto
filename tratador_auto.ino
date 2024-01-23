#include<RTClib.h>
#include<Wire.h>
#include<LiquidCrystal_I2C.h>
#include<stdint.h>

//Variáveis de PRE-configuração
#define TEMPO_MOTOR_LIG 20 //Tempo do Motor ligado por grama de ração

//#define SEM_JOYSTICK

//Display variáveis
#define Display_col 20 // Serve para definir o numero de colunas do display utilizado
#define Display_lin  4 // Serve para definir o numero de linhas do display utilizado
#define ende  0x27 // Serve para definir o endereço do display.



#ifdef SEM_JOYSTICK
  #define botaoCima 6
  #define botaoBaixo 5

  #define botaoEsq 10
  #define botaoDir 9
#else
  #define joyStickVX 0xA1
  #define joyStickVY 0xA0
#endif

#define botao 7

#define qtdaMaxTratamento 7 // Na real são 8 contando a partir do 0
#define qtdaMinTratamento 2 // e o valor minímo é 2 sendo o tratar inicial e o final

typedef struct{
  uint8_t hora;
  int16_t min;
}Time;

typedef struct{
  uint8_t dia;
  uint8_t mes;
  uint16_t ano;
}Data_Lote;

typedef struct{
  unsigned int *qtda; //Quantidade
  unsigned int *peso; //Média do peso
  unsigned int *texCres;  //Taxa de crescimento
  char tipo[8];
}Animais_s;

typedef struct{
  unsigned int *porTrata; //Quantidade de ração por tratamento
  unsigned int *diaria; //Quantidade diaria de ração
  unsigned int *total;  //Total no stock
}Racao_s;

//Variáveis de parâmetros
unsigned int variaveisMenu_1[3]={0,0,0};//Puxar varíaveis da opção no menu e colocar numa array
unsigned int variaveisMenu_2[3]={0,0,0};

//Menu 1
Animais_s animais = {&variaveisMenu_1[0],&variaveisMenu_1[1],&variaveisMenu_1[2],"PEIXES"};
//animais.tipo="PEIXES";

// Menu 2
Racao_s racao = {&variaveisMenu_2[2],&variaveisMenu_2[1],&variaveisMenu_2[0]};

Time horaInicialT, horaFinalT;
uint16_t intervaloTrata;
uint16_t tempoPorTratam;
uint8_t qtdaTratar=7;
uint8_t indexTratar = 7;//index do horário para tratar o peixe


Data_Lote dataLote;

int8_t cursor=0;
uint8_t pagAtual=1;
bool modoSuspenso = false;

//Bibliotecas externas
RTC_DS3231 rtc; //Acessar módulo de RTC(acessar data e hora)
LiquidCrystal_I2C lcd(0x27,20,4); //Acessar o módulo LiquidCrystal(Display) atráves do I2C

//Funções

//  menu
void CarregarMenu_1(); // Desenhar textos base do menu
bool AtualizarMenu_1(); // Atualizar e fazer parte lógica do menu

void CarregarMenu_2();
bool AtualizarMenu_2();

void CarregarMenu_3();
bool AtualizarMenu_3();


void CarregarMenu_DataLote();
bool AtualizarMenu_DataLote();

void CarregarMenu_QTDARefeicao();
bool AtualizarMenu_QTDARefeicao();

void AtualizarMenu();
void Menu();

void (*CarregarMenuAtual)(void);// Ponteiro para função do menu(lógica) que estiver rodando
bool (*AtualizarMenuAtual)(void);// Ponteiro para função do menu(lógica) que estiver rodando

// Funções
bool CheckTratar();
int PerdeuTratar(); // Se passou o horário e não tratou
void ManejarHorarioTratamento(Time *inicio, Time *final, uint8_t qtda); // Orgainzar/Reoganizar os horários do tratamento

void setup() {
  delay(1000);
  
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Incializar RTC
  while(!rtc.begin())Serial.println("DS3231 não encontrado");

  Serial.println("DS3231 encontrado");
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));// Ajustar o horário

  // Incializar LCD
  lcd.init();
  lcd.clear();
  lcd.backlight();

  // Inicializar portas dos botões caso não esteja no modo joystick
  #ifdef SEM_JOYSTICK
    pinMode(botaoCima,INPUT_PULLUP);
    pinMode(botaoBaixo,INPUT_PULLUP);

    pinMode(botaoEsq,INPUT_PULLUP);
    pinMode(botaoDir,INPUT_PULLUP);
    pinMode(botao,INPUT_PULLUP);
  #endif

  //Inicializar o menu incial
  CarregarMenuAtual=&CarregarMenu_1;
  AtualizarMenuAtual=&AtualizarMenu_1;


  //Códigos para testes

  //*animais.qtda=20;
  //Serial.println(*animais.qtda);


  horaInicialT={5,30};
  horaFinalT={22,35};
  ManejarHorarioTratamento(&horaInicialT,&horaFinalT,qtdaTratar);

  Serial.println(checkTratar());
  Serial.println(PerdeuTratar());
}


void loop() {
  /*
  while(modoSuspenso==false){
    Menu();
    Serial.println(*animais.qtda);
  }
  */
  
}

void Suspenso(){
  while(1){

    if(checkTratar()){
      //Tratar
      if(indexTratar == qtdaTratar)indexTratar=0;
      else indexTratar++;
    }
  }
}

//###################################################################################
// Parte dos menus
//###################################################################################

//Funções de chamada
void irPag1(void){
  CarregarMenuAtual=&CarregarMenu_1;
  AtualizarMenuAtual=&AtualizarMenu_1;
  pagAtual=1;
}

void irPag2(void){
  CarregarMenuAtual=&CarregarMenu_2;
  AtualizarMenuAtual=&AtualizarMenu_2;
  pagAtual=2;
}

void irPag3(void){
  CarregarMenuAtual=&CarregarMenu_3;
  AtualizarMenuAtual=&AtualizarMenu_3;
  pagAtual=3;
}

// --------------------
//    Primeiro Menu
// ----------------------

// Desenhar textos bases no display
void CarregarMenu_1(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("QTDA ANI(U):");
  lcd.setCursor(0, 1);
  lcd.print("PESO ANI(Kg):");
  lcd.setCursor(0, 2);
  lcd.print("TAX CRES(%):");

  for(uint8_t i = 0; i < 3; i++){
    AtualizarTelaMenu(variaveisMenu_1[i],i);
  }

}

// Atualizar a variável onde o cursor estiver em cima
void AtualizarTelaMenu(int variavelSelecionada,int8_t cursorPos){
  if(cursorPos==3)return;// não gastar tempo processamento isso

  //Limpar o campo
  lcd.setCursor(12, cursorPos);
  lcd.print("      ");
  lcd.setCursor(12, cursorPos);

  if(pagAtual == 1)
    lcd.print("<"+((cursorPos!=0)?String(float(variavelSelecionada)/1000):String(variavelSelecionada))+">");//Caso opção for 1 que utiliza KG escrever em floats
  else lcd.print("<"+String(variavelSelecionada,DEC)+">");
  return;
}

// Atualizar(lógica) do primeiro menu
bool AtualizarMenu_1(){
  //variaveisMenu_1[cursor]+=((digitalRead(botaoDir)==LOW)-(digitalRead(botaoEsq)==LOW && variaveisMenu_1[cursor]-1 >= 0)) * 10;

  // Alterar para página 2
  if(cursor>2){
    irPag2();
    cursor=0;
    return false;
  }

  if(cursor<0)cursor=0;

  AtualizarTelaMenu(variaveisMenu_1[cursor],cursor);//Rescrever o valor selecionado na tela
  return true;
}

// -------------
// Segundo  Menu
// -------------

void CarregarMenu_2(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("QTDA RAC(G):");
  lcd.setCursor(0, 1);
  lcd.print("RACAO DI(G):");
  lcd.setCursor(0, 2);
  lcd.print("RACAO TR(G):");


  for(uint8_t i = 0; i < 3; i++){
    AtualizarTelaMenu(variaveisMenu_2[i],i);
  }

}
bool AtualizarMenu_2(){
  if(cursor!=2)// Variável 2(ração por tratamento) é imutavel
  //variaveisMenu_2[cursor]+=((digitalRead(botaoDir)==LOW)-(digitalRead(botaoEsq)==LOW && variaveisMenu_1[cursor]-1 >= 0)) * 10;

  // Alterar de tela
  if(cursor>2){// Ir para tela 3
    irPag3();
    cursor=0;
    return false;
  }
  else if(cursor<0){// Voltar para tela 1
    irPag1();
    cursor=3;
    return false;
  }
  
  *racao.porTrata=int(*racao.diaria/qtdaTratar);

  AtualizarTelaMenu(variaveisMenu_2[cursor],cursor);// Atualizar display
  return true;
}

// -------------
// Terceiro  Menu
// -------------

// Desenhar textos bases no display
void CarregarMenu_3(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DATA LOT:"+String(dataLote.dia)+"/"+String(dataLote.mes)+"/"+String(dataLote.ano));
  lcd.setCursor(0, 1);
  lcd.print(F("QTDA REF:"));
  lcd.setCursor(0, 2);
  lcd.print("TIPO:"+String(animais.tipo));
}
/*
bool pedir(const char* texto){
  lcd.setCursor(0, 3);
  lcd.print(texto);

  while(1){
    if(digitalRead(botaoEsq)==LOW)return true;
    else if(digitalRead(botaoDir)==LOW)return false;
  }
}
*/

// Atualizar(lógica) do segundo menu
bool AtualizarMenu_3(){

  if(cursor<0){// Voltar para tela 2
    irPag2();
    cursor=3;
    return false;
  }

  /*
  if(digitalRead(botao)==LOW && cursor == 0){
    switch(cursor){
      case 0:// Opcão DataLote
        EditarNovaDataLote();
      break;
    }
  }
  */

  //Abrir o editor de "dataLote"
  //if(digitalRead(botao)==LOW && cursor == 0)EditarNovaDataLote();

  return true;
}


void Menu(){
  CarregarMenuAtual();


  //Atualizar pagina da tela
  lcd.setCursor(0, 3);
  lcd.print("P:"+String(pagAtual)+"/3");
  
  //Loop do Menu Principal
  while(1){
    int8_t antigoCursorPos = cursor;

    //cursor+=(digitalRead(botaoBaixo)==LOW)-(digitalRead(botaoCima)==LOW && cursor>-1);//Navegar entre as opções

    //Atualizar o menu
    if(AtualizarMenuAtual()==false)break;//Trocar de tela true = igual continuar na mesma tela e false = trocar de tela(sair do loop e renicializar)


    //Atualizar o cursor no menu
    if(cursor!=antigoCursorPos){

      lcd.setCursor(19, antigoCursorPos);
      lcd.print(" ");
      lcd.setCursor(19, cursor);
      lcd.print("*");
    }


    delay(500);
  }

  return;
}

//###################################################################################
// Parte da lógica
//###################################################################################

bool checkTratar(){

  DateTime time = rtc.now();
  uint16_t tempoTotalMinutos = (horaInicialT.hora * 60) + horaInicialT.min;

  tempoTotalMinutos+=intervaloTrata*indexTratar;

  if(tempoTotalMinutos>=time.minute()+(time.hour()*60))return true;
  return false;
}

int PerdeuTratar(){
  DateTime time = rtc.now();
  
  uint16_t tempoTotalMinutos = (horaInicialT.hora* 60) + horaInicialT.min;
  tempoTotalMinutos+=intervaloTrata*indexTratar;
  uint16_t horaMinutos =  time.minute()+( time.hour()*60);

  //Serial.println(String(tempoTotalMinutos)+"/"+String(horaMinutos));

  //Perdeu alguns minutos mas continua na mesma hora pode ser ainda tratado
  if(horaMinutos <= tempoTotalMinutos+60)return 3;//Código para tratar agora

  /*
  if(tratarHora.hora > time.hour()){
    uint8_t nextHora= indexTratar;


    while(1){
      nextHora++;

      //if(nextHora.hora < time.hour() || )
      return 0;
    }
  }
  */
  return 1;
}

void ManejarHorarioTratamento(Time *inicio, Time *final, uint8_t qtda){

  int tempoTotalMinutos = (horaFinalT.hora - horaInicialT.hora) * 60 + (horaFinalT.min - horaInicialT.min);
  // Calcula a duração de cada tratamento em minutos
  intervaloTrata = tempoTotalMinutos / qtdaTratar;
  return;
}