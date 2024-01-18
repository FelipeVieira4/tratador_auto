#include<RTClib.h>
#include<Wire.h>
#include<LiquidCrystal_I2C.h>
#include<stdint.h>

#include<string.h> //ter acesso ao malloc para ponteiros


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

typedef struct{
  uint8_t hora;
  uint8_t minuto;
}hora;

typedef struct{
  uint8_t dia;
  uint8_t mes;
  uint16_t ano;
}data_Lote;

//Variáveis de parâmetros
unsigned int variaveisMenu_1[3]={0,0,0};//Puxar varíaveis da opção no menu e colocar numa array
unsigned int variaveisMenu_2[3]={0,0,0};

// Seria interesante incapsula essa variáveis em structs ou classes

//Menu 1
unsigned int *qtdaAnimais=&variaveisMenu_1[0],*pesoAnimal=&variaveisMenu_1[1];
unsigned int *taxCrescimento=&variaveisMenu_1[2]; //Taxa de crescimento do Animal por dia
//String tipoAnimal;

// Menu 2
unsigned int *racaoDiaria=&variaveisMenu_2[0],*racaoPorRefeica=&variaveisMenu_2[1];
unsigned int *qtdaRacao=&variaveisMenu_2[2];

hora *horariosRefeicoes = NULL;

data_Lote dataLote;
uint8_t qtdaRefeicao;

uint8_t opcaoSelecionada=0;
bool suspenso = false;

//Bibliotecas externas
RTC_DS3231 rtc; //Acessar módulo de RTC(acessar data e hora)
LiquidCrystal_I2C lcd_1(ende,Display_col,Display_lin); //Acessar o módulo LiquidCrystal(Display) atráves do I2C

uint8_t pagAtual=1;

//Funções

// Processos de Aplicação
void CarregarMenu_1(); // Desenhar textos base do menu
bool AtualizarMenu_1(); // Atualizar e fazer parte lógica do menu

void CarregarMenu_2();
bool AtualizarMenu_2();

void CarregarMenu_DataLote();
bool AtualizarMenu_DataLote();

void CarregarMenu_QTDARefeicao();
bool AtualizarMenu_QTDARefeicao();

void AtualizarMenu();
void Menu();

void (*CarregarMenuAtual)(void);//Ponteiro para função do menu(lógica) que estiver rodando
bool (*AtualizarMenuAtual)(void);//Ponteiro para função do menu(lógica) que estiver rodando

void Suspenso();

//void AlterarValor();
//void AtualizaoOpcao();


void setup() {
  delay(1000);
  
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Incializar RTC
  while(!rtc.begin())Serial.println("DS3231 não encontrado");

  Serial.println("DS3231 encontrado");
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));// Ajustar o horário

  // Incializar LCD
  lcd_1.init();
  lcd_1.clear();
  lcd_1.backlight();

  // Inicializar portas dos botões
  pinMode(botaoCima,INPUT_PULLUP);
  pinMode(botaoBaixo,INPUT_PULLUP);

  pinMode(botaoEsq,INPUT_PULLUP);
  pinMode(botaoDir,INPUT_PULLUP);
  pinMode(botao,INPUT_PULLUP);

  //Inicializar o menu incial
  CarregarMenuAtual=&CarregarMenu_1;
  AtualizarMenuAtual=&AtualizarMenu_1;

  *qtdaAnimais=20;
  Serial.println(*qtdaAnimais);
}


void loop() {
  if(suspenso==true){
    Suspenso();
  }else{
    while(suspenso==false){
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

// -------------
// Primeiro Menu
// -------------

// Desenhar textos bases no display
void CarregarMenu_1(){
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("QTDA ANI(U):");
  lcd_1.setCursor(0, 1);
  lcd_1.print("PESO ANI(Kg):");
  lcd_1.setCursor(0, 2);
  lcd_1.print("TAX CRES(%):");

  uint8_t resOpcao = opcaoSelecionada;

  for(uint8_t i = 0; i < 3; i++){
    opcaoSelecionada=i;
    AtualizarTelaMenu(variaveisMenu_1[i]);
  }

  opcaoSelecionada=resOpcao;
  lcd_1.setCursor(19, opcaoSelecionada);
  lcd_1.print("*");

}


// Função para atualizar os dados no display
void AtualizarTelaMenu(int variavelSelecionada){

  lcd_1.setCursor(12, opcaoSelecionada);
  lcd_1.print("      ");
  lcd_1.setCursor(12, opcaoSelecionada);
  lcd_1.print("<"+((opcaoSelecionada!=0 && opcaoSelecionada!=3)?String(float(variavelSelecionada)/1000):String(variavelSelecionada))+">");//Caso opção for 1 que utiliza KG escrever em floats

}

// Atualizar(lógica) do primeiro menu
bool AtualizarMenu_1(){
  variaveisMenu_1[opcaoSelecionada]+=((digitalRead(botaoDir)==LOW)-(digitalRead(botaoEsq)==LOW && variaveisMenu_1[opcaoSelecionada]-1 >= 0)) * 10;

  // Alterar de tela
  if(opcaoSelecionada>2){
    CarregarMenuAtual=&CarregarMenu_2;
    AtualizarMenuAtual=&AtualizarMenu_2;
    pagAtual=2;
    return false;
  }

  AtualizarTelaMenu(variaveisMenu_1[opcaoSelecionada]);//Rescrever o valor selecionado na tela
  return true;
}

// -------------
// Segundo  Menu
// -------------

// Desenhar textos bases no display
void CarregarMenu_2(){
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("DATA LOT:"+String(dataLote.dia)+"/"+String(dataLote.mes)+"/"+String(dataLote.ano));
  lcd_1.setCursor(0, 1);
  lcd_1.print("QTDA REF:");
}

bool pedir(const char* texto){
  int opcao=0;
  while(1){
    lcd_1.setCursor(0, 3);
    lcd_1.print(texto);

    opcao+=((digitalRead(botaoDir)==LOW && opcao < 1)-(digitalRead(botaoEsq)==LOW && opcao > 0));

    if(digitalRead(botaoEsq)==LOW)return true;
    else if(digitalRead(botaoDir)==LOW)return false;
  }
  return false;
}

void EditarNovaDataLote(){
  delay(500);

  if(!pedir("Mudar texto?(Y/n)")){
    lcd_1.setCursor(0, 3);
    lcd_1.print("Data nao alterada");
    return;
  }

  uint8_t posCursos;

  DateTime data = rtc.now();
  unsigned int dados[]={data.day(),data.month(),data.year()};

  while(1){
    lcd_1.setCursor(18, 0);
    lcd_1.print(" ");
    lcd_1.setCursor(9, 0);
    lcd_1.print(String(dados[0])+"/"+dados[1]+"/"+dados[2]);
    

    posCursos+=((digitalRead(botaoDir)==LOW && posCursos < 2)-(digitalRead(botaoEsq)==LOW && posCursos > 0));

    lcd_1.setCursor(19, 0);

    char letra;
    if(posCursos==0)letra='D';
    else if(posCursos==1)letra='M';
    else letra='A';

    lcd_1.print(letra);//Escrever Caracter correpondente a opção selecionada DIA ou MÊS o ANO

    dados[posCursos]+=-(digitalRead(botaoBaixo)==LOW && dados[posCursos]-1 > 0)+(digitalRead(botaoCima)==LOW);

    if(digitalRead(botao)==LOW){
      dataLote.dia=dados[0];
      dataLote.mes=dados[1];
      dataLote.ano=dados[2];

      lcd_1.setCursor(0, 3);
      lcd_1.print("Data alterada    ");
      break;
    }
    delay(500);
  }
}


// Atualizar(lógica) do segundo menu
bool AtualizarMenu_2(){

  uint8_t verdadeiraPosicao=opcaoSelecionada-4;

  if(opcaoSelecionada<4){
    CarregarMenuAtual=&CarregarMenu_1;
    AtualizarMenuAtual=&AtualizarMenu_1;
    pagAtual=1;
    return false;
  }

  
  if(digitalRead(botao)==LOW){
    switch(verdadeiraPosicao){
      case 0:// Opcão DataLote
        EditarNovaDataLote();
      break;
    }
  }

  return true;
}


void Menu(){
  CarregarMenuAtual();


  //Atualizar pagina da tela
  lcd_1.setCursor(0, 3);
  lcd_1.print("P:"+String(pagAtual)+"/2");
  
  //Loop do Menu Principal
  while(1){
    uint8_t antigaOpcaoSel = opcaoSelecionada;

    opcaoSelecionada+=(digitalRead(botaoBaixo)==LOW)-(digitalRead(botaoCima)==LOW && opcaoSelecionada>0);
    //if(digitalRead(botaoBaixo)==LOW && opcaoSelecionada<3)opcaoSelecionada++;
    //else if(digitalRead(botaoCima)==LOW && opcaoSelecionada>0)opcaoSelecionada--;

    //Atualizar Menu
    if(opcaoSelecionada!=antigaOpcaoSel){
      uint8_t redimensiona = (opcaoSelecionada) / 4;

      lcd_1.setCursor(19, antigaOpcaoSel-(redimensiona*4));
      lcd_1.print(" ");
      lcd_1.setCursor(19, opcaoSelecionada-(redimensiona*4));
      lcd_1.print("*");
    }

    if(AtualizarMenuAtual()==false)break;//Trocar de tela

    

    delay(500);
  }

  return;
}