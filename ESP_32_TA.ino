#include <Wire.h>
#include <RtcDS3231.h>
#include <Fuzzy.h>
#include <WiFi.h>
#include <DHT.h>

#define DHTTYPE DHT22
#define DHTPin 33

const int moisture = 34;
const int pompa = 13;
const int ph = 35;

DHT dht(DHTPin, DHTTYPE);
float temp = 0;
float hum = 0;

int nilaiph = 0; //nilai ADC dari sensor
float phTanah = 0.0; //nilai ph

const char* ssid = "D'";
const char* password = "saleho510";
const char* host = "dimasoktafianto.my.id";

//Fuzzy
Fuzzy *fuzzy = new Fuzzy();

//FuzzyInput KelembapanTanah
FuzzySet *kering = new FuzzySet(0, 40, 40, 60);
FuzzySet *lembab = new FuzzySet(40, 60, 60, 80);
FuzzySet *basah = new FuzzySet(60, 80, 80, 100);

//FuzzyInput SuhuUdara
FuzzySet *sejuk = new FuzzySet(14, 24, 24, 36);
FuzzySet *normal = new FuzzySet(24, 36, 36, 48);
FuzzySet *panas = new FuzzySet(36, 48, 48, 58);

//FuzzyOutput Durasi1(1-40HST)
FuzzySet *pendek1 = new FuzzySet(0, 0, 0, 0);
FuzzySet *cukup1 = new FuzzySet(0, 3, 3, 5);
FuzzySet *lama1 = new FuzzySet(3, 5, 6, 6);

//FuzzyOutput Durasi2(41-70HST)
FuzzySet *pendek2 = new FuzzySet(0, 0, 0, 0);
FuzzySet *cukup2 = new FuzzySet(0, 6, 6, 8);
FuzzySet *lama2 = new FuzzySet(8, 10, 12, 12);

RtcDS3231<TwoWire> Rtc(Wire);
byte state;

void setup() {
  Serial.begin(9600);
  delay(50);

  dht.begin();
  pinMode(pompa, OUTPUT);
  setupFuzzy();

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  Rtc.Begin();
  delay(5000);
  state = 0; //kondisi awal
}

void loop() {
  switch (state)
  {
    case 0:
            bacaRTC();
            break;
    case 1:
            bacaUmur();
            break;
    case 2:
            monitoring();
            jeda();
            state = 0;
            break;
    case 3:
            penyiramanTahap1();
            jeda();
            state = 2;
            break;
    case 4:
            penyiramanTahap2();
            jeda();
            state = 2;
            break;
  }
}

void jeda() {
  int jedawaktu = (30*(60*1000));
  delay(jedawaktu);
//  state=0;
}

void bacaRTC() {
  RtcDateTime currentTime = Rtc.GetDateTime();

//  char str[15];
//  sprintf(str, "%d/%d/%d %d:%d:%d", 
//            currentTime.Year(),
//            currentTime.Month(),
//            currentTime.Day(),
//            currentTime.Hour(),
//            currentTime.Minute(),
//            currentTime.Second()
//         );
         
//  Serial.print("Jam Saat Ini : ");
//  Serial.print(currentTime.Hour() + ":");
//  Serial.print(currentTime.Minute() + ":");
//  Serial.print(currentTime.Second());
//  Serial.println();
  
  if (currentTime.Hour() == 15) {
    Serial.println("Waktunya Menyiram Tanaman");
    state = 1;
  }else {
    Serial.println("Belum Saatnya Menyiram");
    state = 2;
  }
}

void bacaUmur() {
  RtcDateTime currentTime = Rtc.GetDateTime();
  
  int x = dateDiff(2020,5,19, currentTime.Year(),currentTime.Month(),currentTime.Day());
  Serial.print("Umur : ");
  Serial.print(x);
  Serial.println(" Hari");
//  delay(10000);

  if (x <= 40) {
    Serial.println("Melakukan Penyiraman Tahap 1");
    state = 3;
  }else if (x >= 41) {
    Serial.println("Melakukan Penyiraman Tahap 2");
    state = 4;
  }
}

void monitoring() {
  delay(2000);
  RtcDateTime currentTime = Rtc.GetDateTime(); 
  int x = dateDiff(2020,5,19, currentTime.Year(),currentTime.Month(),currentTime.Day());

  int kelTanah;
  int nilaiADC = analogRead(moisture);
  kelTanah = (100-((nilaiADC/4095.00)*100));

  temp = dht.readTemperature();
  hum = dht.readHumidity();

  nilaiph = analogRead(ph);
  phTanah = (0.01725*nilaiph)+2.4618;

  Serial.print("Kelembapan Tanah : ");
  Serial.print(kelTanah);
  Serial.println(" %");
  Serial.print("Suhu : ");
  Serial.print(temp);
  Serial.println(" C");
  Serial.print("Kelembapan Udara : ");
  Serial.print(hum);
  Serial.println(" %");
  Serial.print("ph Tanah : ");
  Serial.print(phTanah);
  Serial.println(" %");
  Serial.print("Umur Tanaman : ");
  Serial.print(x);
  Serial.println(" Hari");

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(host);

  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection Failed");
    return;
  }

  //We now create a URL for the request
  String url = "/tugasakhir/add.php?";
  url += "kelembabantanah=";
  url += kelTanah;
  url += "&suhu=";
  url += temp;
  url += "&humidity=";
  url += hum;
  url += "&ph=";
  url += phTanah;
  url += "&durasi1=";
  url += "";
  url += "&durasi2=";
  url += "";
  url += "&age=";
  url += x;
  url += "&time=";
  url += "";

  Serial.print("Requesting URL : ");
  Serial.println(url);

  //This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  //Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.println(line);

    if (line.indexOf("sukses gaes") != -1) {
      Serial.println();
      Serial.println("Yes, data masuk");
    }else if (line.indexOf("gagal gaes") != -1) {
      Serial.println();
      Serial.println("Maaf, data gagal masuk");
    }
  }
}

void penyiramanTahap1() {
  delay(2000);
  RtcDateTime currentTime = Rtc.GetDateTime(); 
  int x = dateDiff(2020,5,19, currentTime.Year(),currentTime.Month(),currentTime.Day());
  
//  char jam[15];
//  sprintf(jam, "%d:%d:%d", 
//            currentTime.Hour(),
//            currentTime.Minute(),
//            currentTime.Second()
//         );
  
  int kelTanah;
  int nilaiADC = analogRead(moisture);
  kelTanah = (100-((nilaiADC/4095.00)*100));

  temp = dht.readTemperature();
  hum = dht.readHumidity();

  nilaiph = analogRead(ph);
  phTanah = (0.01725*nilaiph)+2.4618;

  Serial.print("Kelembapan Tanah : ");
  Serial.print(kelTanah);
  Serial.println(" %");
  Serial.print("Suhu : ");
  Serial.print(temp);
  Serial.println(" C");
  Serial.print("Kelembapan Udara : ");
  Serial.print(hum);
  Serial.println(" %");
  Serial.print("ph Tanah : ");
  Serial.print(phTanah);
  Serial.println(" %");
  Serial.print("Umur Tanaman : ");
  Serial.print(x);
  Serial.println(" Hari");
  
  fuzzy->setInput(1, kelTanah);
  fuzzy->setInput(2, temp);

  fuzzy->fuzzify();
  float output1 = fuzzy->defuzzify(1);
  Serial.print("Durasi Penyiraman : ");
  Serial.print(output1);
  Serial.println(" Detik");

  int semprot = output1*1000;
  digitalWrite(pompa, HIGH);
  delay(semprot);
  digitalWrite(pompa, LOW);

  Serial.println();
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(host);

  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  //We now create a URL for the request
  String url = "/tugasakhir/add.php?";
  url += "kelembabantanah=";
  url += kelTanah;
  url += "&suhu=";
  url += temp;
  url += "&humidity=";
  url += hum;
  url += "&ph=";
  url += phTanah;
  url += "&durasi1=";
  url += output1;
  url += "&durasi2=";
  url += "";
  url += "&age=";
  url += x;
  url += "&time=";
  url += "";

  Serial.print("Requesting URL: ");
  Serial.println(url);

  //This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  //Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.println(line);

    if (line.indexOf("sukses gaes") != -1) {
      Serial.println();
      Serial.println("Yes, data masuk");
    }else if (line.indexOf("gagal gaes") != -1) {
      Serial.println();
      Serial.println("Maaf, data gagal masuk");
    }
  }
}

void penyiramanTahap2() {
  delay(2000);
  RtcDateTime currentTime = Rtc.GetDateTime(); 
  int x = dateDiff(2020,5,19, currentTime.Year(),currentTime.Month(),currentTime.Day());

//  char jam[15];
//  sprintf(jam, "%d:%d:%d", 
//            currentTime.Hour(),
//            currentTime.Minute(),
//            currentTime.Second()
//         );
  
  int kelTanah;
  int nilaiADC = analogRead(moisture);
  kelTanah = (100-((nilaiADC/4095.00)*100));

  temp = dht.readTemperature();
  hum = dht.readHumidity();

  nilaiph = analogRead(ph);
  phTanah = (0.01725*nilaiph)+2.4618;

  Serial.print("Kelembapan Tanah : ");
  Serial.print(kelTanah);
  Serial.println(" %");
  Serial.print("Suhu : ");
  Serial.print(temp);
  Serial.println(" C");
  Serial.print("Kelembapan Udara : ");
  Serial.print(hum);
  Serial.println(" %");
  Serial.print("ph Tanah : ");
  Serial.print(phTanah);
  Serial.println(" %");
  Serial.print("Umur Tanaman : ");
  Serial.print(x);
  Serial.println(" Hari");
  
  fuzzy->setInput(1, kelTanah);
  fuzzy->setInput(2, temp);

  fuzzy->fuzzify();
  float output2 = fuzzy->defuzzify(2);
  Serial.print("Durasi Penyiraman : ");
  Serial.print(output2);
  Serial.println(" Detik");

  int semprot = output2*1000;
  digitalWrite(pompa, HIGH);
  delay(semprot);
  digitalWrite(pompa, LOW);

  Serial.println();
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(host);

  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  //We now create a URL for the request
  String url = "/tugasakhir/add.php?";
  url += "kelembabantanah=";
  url += kelTanah;
  url += "&suhu=";
  url += temp;
  url += "&humidity=";
  url += hum;
  url += "&ph=";
  url += phTanah;
  url += "&durasi1=";
  url += "";
  url += "&durasi2=";
  url += output2;
  url += "&age=";
  url += x;
  url += "&time=";
  url += "";

  Serial.print("Requesting URL: ");
  Serial.println(url);

  //This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  //Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.println(line);

    if (line.indexOf("sukses gaes") != -1) {
      Serial.println();
      Serial.println("Yes, data masuk");
    }else if (line.indexOf("gagal gaes") != -1) {
      Serial.println();
      Serial.println("Maaf, data gagal masuk");
    }
  }
}

int dateDiff(int year1, int mon1, int day1, int year2, int mon2, int day2) {
  int ref,dd1,dd2,i;
  ref = year1;
  if(year2<year1)
  ref = year2;
  dd1=0;
  dd1=dater(mon1);
  for(i=ref;i<year1;i++) {
    if(i%4==0)
    dd1+=1;
  }
  dd1=dd1+day1+(year1-ref)*365;
  dd2=0;
  for(i=ref;i<year2;i++) {
    if(i%4==0)
    dd2+=1;
  }
  dd2=dater(mon2)+dd2+day2+((year2-ref)*365);
  return dd2-dd1;
}

int dater(int x) {
  const int dr[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  return dr[x-1];
}

void setupFuzzy() {
  //FuzzyInputKelembapanTanah
  FuzzyInput *kelembapan = new FuzzyInput(1);
  kelembapan->addFuzzySet(kering);
  kelembapan->addFuzzySet(lembab);
  kelembapan->addFuzzySet(basah);
  fuzzy->addFuzzyInput(kelembapan);

  //FuzzyInputSuhuUdara
  FuzzyInput *suhu = new FuzzyInput(2);
  suhu->addFuzzySet(sejuk);
  suhu->addFuzzySet(normal);
  suhu->addFuzzySet(panas);
  fuzzy->addFuzzyInput(suhu);

  //FuzzyOutputDurasi-1
  FuzzyOutput *durasi1 = new FuzzyOutput(1);
  durasi1->addFuzzySet(pendek1);
  durasi1->addFuzzySet(cukup1);
  durasi1->addFuzzySet(lama1);
  fuzzy->addFuzzyOutput(durasi1);

  //FuzzyOutputDurasi-2
  FuzzyOutput *durasi2 = new FuzzyOutput(2);
  durasi2->addFuzzySet(pendek2);
  durasi2->addFuzzySet(cukup2);
  durasi2->addFuzzySet(lama2);
  fuzzy->addFuzzyOutput(durasi2);

  //BuildingFuzzyUmurTanaman1-40HST
  //FuzzyRule1-->if kering and sejuk then lama1
  FuzzyRuleAntecedent *ifKelembapanKeringAndSuhuSejuk = new FuzzyRuleAntecedent();
  ifKelembapanKeringAndSuhuSejuk->joinWithAND(kering, sejuk);
  FuzzyRuleConsequent *thenDurasiLama1 = new FuzzyRuleConsequent();
  thenDurasiLama1->addOutput(lama1);
  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, ifKelembapanKeringAndSuhuSejuk, thenDurasiLama1);
  fuzzy->addFuzzyRule(fuzzyRule1);

  //FuzzyRule2-->if lembab and sejuk then cukup1
  FuzzyRuleAntecedent *ifKelembapanLembabAndSuhuSejuk = new FuzzyRuleAntecedent();
  ifKelembapanLembabAndSuhuSejuk->joinWithAND(lembab, sejuk);
  FuzzyRuleConsequent *thenDurasiCukup1 = new FuzzyRuleConsequent();
  thenDurasiCukup1->addOutput(cukup1);
  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, ifKelembapanLembabAndSuhuSejuk, thenDurasiCukup1);
  fuzzy->addFuzzyRule(fuzzyRule2);

  //FuzzyRule3-->if basah and sejuk then pendek1
  FuzzyRuleAntecedent *ifKelembapanBasahAndSuhuSejuk = new FuzzyRuleAntecedent();
  ifKelembapanBasahAndSuhuSejuk->joinWithAND(basah, sejuk);
  FuzzyRuleConsequent *thenDurasiPendek1 = new FuzzyRuleConsequent();
  thenDurasiPendek1->addOutput(pendek1);
  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, ifKelembapanBasahAndSuhuSejuk, thenDurasiPendek1);
  fuzzy->addFuzzyRule(fuzzyRule3);

  //FuzzyRule4-->if kering and normal then lama1
  FuzzyRuleAntecedent *ifKelembapanKeringAndSuhuNormal = new FuzzyRuleAntecedent();
  ifKelembapanKeringAndSuhuNormal->joinWithAND(kering, normal);
  FuzzyRuleConsequent *thenDurasiLama2 = new FuzzyRuleConsequent();
  thenDurasiLama2->addOutput(lama1);
  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, ifKelembapanKeringAndSuhuNormal, thenDurasiLama2);
  fuzzy->addFuzzyRule(fuzzyRule4);

  //FuzzyRule5-->if lembab and normal then cukup1
  FuzzyRuleAntecedent *ifKelembapanLembabAndSuhuNormal = new FuzzyRuleAntecedent();
  ifKelembapanLembabAndSuhuNormal->joinWithAND(lembab, normal);
  FuzzyRuleConsequent *thenDurasiCukup2 = new FuzzyRuleConsequent();
  thenDurasiCukup2->addOutput(cukup1);
  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, ifKelembapanLembabAndSuhuNormal, thenDurasiCukup2);
  fuzzy->addFuzzyRule(fuzzyRule5);

  //FuzzyRule6-->if basah and normal then pendek1
  FuzzyRuleAntecedent *ifKelembapanBasahAndSuhuNormal = new FuzzyRuleAntecedent();
  ifKelembapanBasahAndSuhuNormal->joinWithAND(basah, normal);
  FuzzyRuleConsequent *thenDurasiPendek2 = new FuzzyRuleConsequent();
  thenDurasiPendek2->addOutput(pendek1);
  FuzzyRule *fuzzyRule6 = new FuzzyRule(6, ifKelembapanBasahAndSuhuNormal, thenDurasiPendek2);
  fuzzy->addFuzzyRule(fuzzyRule6);

  //FuzzyRule7-->if kering and panas then lama1
  FuzzyRuleAntecedent *ifKelembapanKeringAndSuhuPanas = new FuzzyRuleAntecedent();
  ifKelembapanKeringAndSuhuPanas->joinWithAND(kering, panas);
  FuzzyRuleConsequent *thenDurasiLama3 = new FuzzyRuleConsequent();
  thenDurasiLama3->addOutput(lama1);
  FuzzyRule *fuzzyRule7 = new FuzzyRule(7, ifKelembapanKeringAndSuhuPanas, thenDurasiLama3);
  fuzzy->addFuzzyRule(fuzzyRule7);

  //FuzzyRule8-->if lembab and panas then cukup1
  FuzzyRuleAntecedent *ifKelembapanLembabAndSuhuPanas = new FuzzyRuleAntecedent();
  ifKelembapanLembabAndSuhuPanas->joinWithAND(lembab, panas);
  FuzzyRuleConsequent *thenDurasiCukup3 = new FuzzyRuleConsequent();
  thenDurasiCukup3->addOutput(cukup1);
  FuzzyRule *fuzzyRule8 = new FuzzyRule(8, ifKelembapanLembabAndSuhuPanas, thenDurasiCukup3);
  fuzzy->addFuzzyRule(fuzzyRule8);

  //FuzzyRule9-->if basah and panas then pendek1
  FuzzyRuleAntecedent *ifKelembapanBasahAndSuhuPanas = new FuzzyRuleAntecedent();
  ifKelembapanBasahAndSuhuPanas->joinWithAND(basah, panas);
  FuzzyRuleConsequent *thenDurasiPendek3 = new FuzzyRuleConsequent();
  thenDurasiPendek3->addOutput(pendek1);
  FuzzyRule *fuzzyRule9 = new FuzzyRule(9, ifKelembapanBasahAndSuhuPanas, thenDurasiPendek3);
  fuzzy->addFuzzyRule(fuzzyRule9);



  //BuildingFuzzyUmurTanaman41-70HST
  //FuzzyRule10-->if kering and sejuk then lama2
  FuzzyRuleAntecedent *ifKelembapanKeringAndSuhuSejuk2 = new FuzzyRuleAntecedent();
  ifKelembapanKeringAndSuhuSejuk2->joinWithAND(kering, sejuk);
  FuzzyRuleConsequent *thenDurasiLama4 = new FuzzyRuleConsequent();
  thenDurasiLama4->addOutput(lama2);
  FuzzyRule *fuzzyRule10 = new FuzzyRule(10, ifKelembapanKeringAndSuhuSejuk2, thenDurasiLama4);
  fuzzy->addFuzzyRule(fuzzyRule10);

  //FuzzyRule11-->if lembab and sejuk then cukup2
  FuzzyRuleAntecedent *ifKelembapanLembabAndSuhuSejuk2 = new FuzzyRuleAntecedent();
  ifKelembapanLembabAndSuhuSejuk2->joinWithAND(lembab, sejuk);
  FuzzyRuleConsequent *thenDurasiCukup4 = new FuzzyRuleConsequent();
  thenDurasiCukup4->addOutput(cukup2);
  FuzzyRule *fuzzyRule11 = new FuzzyRule(11, ifKelembapanLembabAndSuhuSejuk2, thenDurasiCukup4);
  fuzzy->addFuzzyRule(fuzzyRule11);

  //FuzzyRule12-->if basah and sejuk then pendek2
  FuzzyRuleAntecedent *ifKelembapanBasahAndSuhuSejuk2 = new FuzzyRuleAntecedent();
  ifKelembapanBasahAndSuhuSejuk2->joinWithAND(basah, sejuk);
  FuzzyRuleConsequent *thenDurasiPendek4 = new FuzzyRuleConsequent();
  thenDurasiPendek4->addOutput(pendek2);
  FuzzyRule *fuzzyRule12 = new FuzzyRule(12, ifKelembapanBasahAndSuhuSejuk2, thenDurasiPendek4);
  fuzzy->addFuzzyRule(fuzzyRule12);

  //FuzzyRule13-->if kering and normal then lama2
  FuzzyRuleAntecedent *ifKelembapanKeringAndSuhuNormal2 = new FuzzyRuleAntecedent();
  ifKelembapanKeringAndSuhuNormal2->joinWithAND(kering, normal);
  FuzzyRuleConsequent *thenDurasiLama5 = new FuzzyRuleConsequent();
  thenDurasiLama5->addOutput(lama2);
  FuzzyRule *fuzzyRule13 = new FuzzyRule(13, ifKelembapanKeringAndSuhuNormal2, thenDurasiLama5);
  fuzzy->addFuzzyRule(fuzzyRule13);

  //FuzzyRule14-->if lembab and normal then cukup2
  FuzzyRuleAntecedent *ifKelembapanLembabAndSuhuNormal2 = new FuzzyRuleAntecedent();
  ifKelembapanLembabAndSuhuNormal2->joinWithAND(lembab, normal);
  FuzzyRuleConsequent *thenDurasiCukup5 = new FuzzyRuleConsequent();
  thenDurasiCukup5->addOutput(cukup2);
  FuzzyRule *fuzzyRule14 = new FuzzyRule(14, ifKelembapanLembabAndSuhuNormal2, thenDurasiCukup5);
  fuzzy->addFuzzyRule(fuzzyRule14);

  //FuzzyRule15-->if basah and normal then pendek2
  FuzzyRuleAntecedent *ifKelembapanBasahAndSuhuNormal2 = new FuzzyRuleAntecedent();
  ifKelembapanBasahAndSuhuNormal2->joinWithAND(basah, normal);
  FuzzyRuleConsequent *thenDurasiPendek5 = new FuzzyRuleConsequent();
  thenDurasiPendek5->addOutput(pendek2);
  FuzzyRule *fuzzyRule15 = new FuzzyRule(15, ifKelembapanBasahAndSuhuNormal2, thenDurasiPendek5);
  fuzzy->addFuzzyRule(fuzzyRule15);

  //FuzzyRule16-->if kering and panas then lama2
  FuzzyRuleAntecedent *ifKelembapanKeringAndSuhuPanas2 = new FuzzyRuleAntecedent();
  ifKelembapanKeringAndSuhuPanas2->joinWithAND(kering, panas);
  FuzzyRuleConsequent *thenDurasiLama6 = new FuzzyRuleConsequent();
  thenDurasiLama6->addOutput(lama2);
  FuzzyRule *fuzzyRule16 = new FuzzyRule(16, ifKelembapanKeringAndSuhuPanas2, thenDurasiLama6);
  fuzzy->addFuzzyRule(fuzzyRule16);

  //FuzzyRule17-->if lembab and panas then cukup2
  FuzzyRuleAntecedent *ifKelembapanLembabAndSuhuPanas2 = new FuzzyRuleAntecedent();
  ifKelembapanLembabAndSuhuPanas2->joinWithAND(lembab, panas);
  FuzzyRuleConsequent *thenDurasiCukup6 = new FuzzyRuleConsequent();
  thenDurasiCukup6->addOutput(cukup2);
  FuzzyRule *fuzzyRule17 = new FuzzyRule(17, ifKelembapanLembabAndSuhuPanas2, thenDurasiCukup6);
  fuzzy->addFuzzyRule(fuzzyRule17);

  //FuzzyRule18-->if basah and panas then pendek2
  FuzzyRuleAntecedent *ifKelembapanBasahAndSuhuPanas2 = new FuzzyRuleAntecedent();
  ifKelembapanBasahAndSuhuPanas2->joinWithAND(basah, panas);
  FuzzyRuleConsequent *thenDurasiPendek6 = new FuzzyRuleConsequent();
  thenDurasiPendek6->addOutput(pendek2);
  FuzzyRule *fuzzyRule18 = new FuzzyRule(18, ifKelembapanBasahAndSuhuPanas2, thenDurasiPendek6);
  fuzzy->addFuzzyRule(fuzzyRule18);
}


