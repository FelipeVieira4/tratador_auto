#include<RTClib.h>
#include<Wire.h>
#include<LiquidCrystal_I2C.h>
#include<stdint.h>

//Variáveis de PRE-configuração
#define TEMPO_MOTOR_LIG 20 //Tempo do Motor ligado por grama de ração

//Display variáveis
#define Display_col 20 // Serve para definir o numero de colunas do display utilizado
#define Display_lin  4 // Serve para definir o numero de linhas do display utilizado
#define ende  0x27 // Serve para definir o endereço do display.

#define botaoCima 6
#define botaoBaixo 5

#define botaoEsq 10
#define botaoDir 9

#define botao 12

#define qtdaMaxTratamento 7 // Na real são 8 contando a partir do 0
#define qtdaMinTratamento 2 // e o valor minímo é 2 sendo o tratar inicial e o final

typedef struct{
  uint8_t hora;
  int16_t minuto;
}Hora;

typedef struct{
  uint8_t dia;
  uint8_t mes;
  uint16_t ano;
}Data_Lote;

//Variáveis de parâmetros
unsigned int variaveisMenu_1[3]={0,0,0};//Puxar varíaveis da opção no menu e colocar numa array
unsigned int variaveisMenu_2[3]={0,0,0};

// Seria interesante encapsular essa variáveis em structs ou classes

//Menu 1
unsigned int *qtdaAnimais=&variaveisMenu_1[0],*pesoAnimal=&variaveisMenu_1[1];
unsigned int *taxCres=&variaveisMenu_1[2]; //Taxa de crescimento do Animal por dia
//String tipoAnimal;

// Menu 2
unsigned int *racaoDiaria=&variaveisMenu_2[1],*racaoTratar=&variaveisMenu_2[2];
unsigned int *qtdaRacao=&variaveisMenu_2[0];

Hora horariosRefeicoes[qtdaMaxTratamento];
Hora horaInicialT, horaFinalT;

Data_Lote dataLote;
uint8_t qtdaTratar=7;
uint8_t horaTratar = 0;//index do horário para tratar o peixe

int8_t cursor=0;
bool modoSuspenso = false;

//Bibliotecas externas
RTC_DS3231 rtc; //Acessar módulo de RTC(acessar data e hora)
LiquidCrystal_I2C lcd(ende,Display_col,Display_lin); //Acessar o módulo LiquidCrystal(Display) atráves do I2C

uint8_t pagAtual=1;

//Funções

// Funções do menu
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

void (*CarregarMenuAtual)(void);//Ponteiro para função do menu(lógica) que estiver rodando
bool (*AtualizarMenuAtual)(void);//Ponteiro para função do menu(lógica) que estiver rodando

// Funções
void ManejarHorarioTratamento();

void Suspenso();


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

  // Inicializar portas dos botões
  pinMode(botaoCima,INPUT_PULLUP);
  pinMode(botaoBaixo,INPUT_PULLUP);

  pinMode(botaoEsq,INPUT_PULLUP);
  pinMode(botaoDir,INPUT_PULLUP);
  pinMode(botao,INPUT_PULLUP);

  //Inicializar o menu incial
  CarregarMenuAtual=&CarregarMenu_1;
  AtualizarMenuAtual=&AtualizarMenu_1;

  //*qtdaAnimais=20;
  //Serial.println(*qtdaAnimais);
  horaInicialT={5,30};
  horaFinalT={18,45};
  ManejarHorarioTratamento();
}


void loop() {
  if(modoSuspenso==true){
    Suspenso();
  }else{
    while(modoSuspenso==false){
      Menu();
      Serial.println(*qtdaAnimais);
    }
  }

}

void Suspenso(){
  /*
  DateTime agora = rtc.now(); // Faz a leitura de dados de data e hora
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("DIA:"+String(agora.day(), DEC));
  lcd_1.setCursor(0, 1);
  lcd_1.print("MES:"+String(agora.month(), DEC));
  lcd_1.setCursor(0, 2);
  lcd_1.print("ANO:"+String(agora.year(), DEC));
  lcd_1.setCursor(0, 3);
  lcd_1.print("HORA:"+String(agora.hour(), DEC)+":"+String(agora.minute(), DEC)+":"+String(agora.second(), DEC));
  delay(1000);
  */
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
  variaveisMenu_1[cursor]+=((digitalRead(botaoDir)==LOW)-(digitalRead(botaoEsq)==LOW && variaveisMenu_1[cursor]-1 >= 0)) * 10;

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
  variaveisMenu_2[cursor]+=((digitalRead(botaoDir)==LOW)-(digitalRead(botaoEsq)==LOW && variaveisMenu_1[cursor]-1 >= 0)) * 10;

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
  
  *racaoTratar=int(*racaoDiaria/qtdaTratar);

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
}

bool pedir(const char* texto){
  lcd.setCursor(0, 3);
  lcd.print(texto);

  while(1){
    if(digitalRead(botaoEsq)==LOW)return true;
    else if(digitalRead(botaoDir)==LOW)return false;
  }
}

void EditarNovaDataLote(){
  delay(500);

  // pedir para o úsuario se ele deseja trocar o dataLote
  if(!pedir("Mudar Data?(Y/n)")){
    lcd.setCursor(0, 3);
    lcd.print("Data nao alterada");
    return;
  }

  uint8_t posCursos;// posição do cursor para editar dia/mês/ano

  DateTime data = rtc.now();// atualizar para data atual
  unsigned int dados[]={data.day(),data.month(),data.year()};

  while(1){

    //char dataString[11];//String data para formatar
    //snprintf(dataString, sizeof(dataString), "%02d/%02d/%04d", dados[0], dados[1], dados[2]);

    lcd.setCursor(9, 0);
    lcd.print(String(dados[0], DEC) + "/" + String(dados[1], DEC) + "/" + String(dados[2], DEC));
    

    posCursos+=((digitalRead(botaoDir)==LOW && posCursos < 2)-(digitalRead(botaoEsq)==LOW && posCursos > 0));

    // limpar o o final data
    lcd.setCursor(17, 0);
    lcd.print("  ");

    char letra;
    if(posCursos==0)letra='D';
    else if(posCursos==1)letra='M';
    else letra='A';

    lcd.print(letra);//Escrever Caracter correpondente a opção selecionada DIA ou MÊS o ANO

    dados[posCursos]+=-(digitalRead(botaoBaixo)==LOW && dados[posCursos]-1 > 0)+(digitalRead(botaoCima)==LOW);

    //Setar o novo ano ao dataLote
    if(digitalRead(botao)==LOW){
      dataLote.dia=dados[0];
      dataLote.mes=dados[1];
      dataLote.ano=dados[2];

      //Escrever que a data for alterada com sucesso e sair do loop(voltar ao menu)
      lcd.setCursor(0, 3);
      lcd.print("Data alterada    ");

      break;
    }
    delay(500);
  }
}


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
  if(digitalRead(botao)==LOW && cursor == 0)EditarNovaDataLote();

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

    cursor+=(digitalRead(botaoBaixo)==LOW)-(digitalRead(botaoCima)==LOW && cursor>-1);//Navegar entre as opções

    //if(digitalRead(botaoBaixo)==LOW && opcaoSelecionada<3)opcaoSelecionada++;
    //else if(digitalRead(botaoCima)==LOW && opcaoSelecionada>0)opcaoSelecionada--;

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

void ManejarHorarioTratamento(){

  int tempoTotalMinutos = (horaFinalT.hora - horaInicialT.hora) * 60 + (horaFinalT.minuto - horaInicialT.minuto);
  // Calcula a duração de cada tratamento em minutos
  int duracaoTratamento = tempoTotalMinutos / qtdaTratar;

  Serial.println("Duração de cada tratamento: " + String(duracaoTratamento) + " minutos");

  Hora horarioAtual = horaInicialT;

  horariosRefeicoes[0]=horaInicialT;//Primeiro tratamento é na hora inicial

  //Começar a setar os outros horários
  for (int i = 1; i <= qtdaTratar-1; i++) {//qtdaTratar-1; pular último valor por que esse nós ja sabemos

    // Avança para o próximo horário com base na duração do tratamento
    horarioAtual.minuto += duracaoTratamento;

    // Ajusta a hora e minuto
    horarioAtual.hora += horarioAtual.minuto / 60;
    horarioAtual.minuto %= 60;

    // Ajusta a hora se ultrapassar meia-noite
    horarioAtual.hora%=24;

    horariosRefeicoes[i]=horarioAtual;
  }
  horariosRefeicoes[qtdaTratar]=horaFinalT;// setar o último vamos

  //Loop para printar no Serial
  /*
  for (int i = 0; i <= qtdaTratar; i++) {
    Serial.println("Tratamento " + String(i) + ": " + String( horariosRefeicoes[i].hora) + ":" + String( horariosRefeicoes[i].minuto));
  }
  */
}
