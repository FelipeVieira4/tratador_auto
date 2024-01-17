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

typedef struct{
  uint8_t hora;
  uint8_t minuto;
}hora;

//Variáveis de parâmetros
unsigned int qtdaAnimais;
String tipoAnimal;
float pesoAnimal;
float taxCrescimento; //Taxa de crescimento do Animal por dia

unsigned int racaoDiaria,racaoPorRefeicao;
unsigned int qtdaRacao;

hora *horariosRefeicoes = NULL;

uint8_t qtdaRefeicao;

//enum opcao{qtd_animais=0,peso_animal,menu_refeicoes,racao_diaria};//Opções no Menu
uint8_t opcaoSelecionada=0;
bool suspenso = false;

//Bibliotecas externas
RTC_DS3231 rtc; //Acessar módulo de RTC(acessar data e hora)
LiquidCrystal_I2C lcd_1(ende,Display_col,Display_lin); //Acessar o módulo LiquidCrystal(Display) atráves do I2C


//Funções
void SuspensoAtivado();
void SuspensoDesativado();

void DesenharMenuPrincipal();
void MenuPrincipal();

void DesenharMenuRefeicao();

void AlterarValor();

void AtualizaoOpcao();

void (*desenhar)(void);//Ponteiro para função de desenho
void (*menu)(void);//Ponteiro para função do menu(lógica)

void setup() {
  delay(1000);
  
  // put your setup code here, to run once:
  Serial.begin(9600);
  while(!rtc.begin())Serial.println("DS3231 não encontrado");

  Serial.println("DS3231 encontrado");
  
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));// Ajustar o horário
  
  lcd_1.init();
  lcd_1.clear();
  lcd_1.backlight();

  pinMode(botaoCima,INPUT_PULLUP);
  pinMode(botaoBaixo,INPUT_PULLUP);

  pinMode(botaoEsq,INPUT_PULLUP);
  pinMode(botaoDir,INPUT_PULLUP);

  desenhar=&DesenharMenuPrincipal;
  menu=&MenuPrincipal;
}

void loop() {
  SuspensoDesativado();
}

void SuspensoAtivado(){
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
}

void SuspensoDesativado(){
  desenhar();
  AtualizaoOpcao();
}

void DesenharMenuPrincipal(){
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("QTDA ANI: <"+String(qtdaAnimais)+">");
  lcd_1.setCursor(0, 1);
  lcd_1.print("PESO ANI: <"+String(pesoAnimal)+">");
  lcd_1.setCursor(0, 2);
  lcd_1.print("RACAO DIA: <"+String(racaoDiaria)+">");
  lcd_1.setCursor(0, 3);
  lcd_1.print("REFEICOES");

  lcd_1.setCursor(19, opcaoSelecionada);
  lcd_1.print("*");
  delay(200);
}
void MenuPrincipal(){
  if(digitalRead(botaoBaixo)==LOW && opcaoSelecionada<3)opcaoSelecionada++;
  if(digitalRead(botaoCima)==LOW && opcaoSelecionada>0)opcaoSelecionada--;

  
}
