# Greenhouse-project
Remote temperature and humidity greenhouse control, with master and slaves

/*Project Domotic Greenhouse*/

/*Autors: La Mura Antonio, Festa Umberto*/

/*Master*/

/*Temperature and humidity control with DHT22 sensor*/
/*Master communicates with webserver via WiFi, using CC3000 shield. It sends datas which are received from Slave, to DataBase. (You can see how it works on www.cormaz.altervista.org)*/
/*Master receives datas from Slave via Wireless at 2.4[GHz], with NFR24L01 sensor and if it is necessary, it can send new thresholds of temperature and humidity*/

/*Slave*/

/*Temperature and humidity control with DHT22 sensor*/
/*Slave reads humidity and temperature in greenhouse, then it calculates CRC-8 with Dallas/Maxim's formulas and finally it uses that to ensure that any information is lost*/
/*Slave sends datas to Master via Wireless at 2.4[GHz], with NFR24L01 sensor and it receives from Master new settings, new thresholds of temperature and humidity*/
/*Slave compares the sensor's values with thresholds and it turns on or turns off the HEATER or the HUMIDIFIER or the FAN*/

/*Master: Arduino Mega - Slave: Arduino UNO*/
