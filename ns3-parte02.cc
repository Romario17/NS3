//Alunos: Bruno César e Romário Fernando

#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/config-store-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-module.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace ns3;

void simulacao(uint32_t qtd_nos, uint32_t area, int posicionamento_nos[][2]);

int main(int argc, char **argv){
  
  CommandLine cmd;
  cmd.Parse (argc, argv);

  int i;
  srand(time(NULL));
  
  int area = 700;
  int qtd_nos = 10;
  int posicionamento_nos[qtd_nos][2];

  for(i=0;i<qtd_nos;i++){//posicionamento dos nos sobre a area definida

		posicionamento_nos[i][0] = rand()%area;
		posicionamento_nos[i][1] = rand()%area;
	
  }
  
  simulacao(qtd_nos,area,posicionamento_nos);
	
	return 0;
}

void simulacao(uint32_t qtd_nos, uint32_t area,  int posicionamento_nos[][2]){
  
  //uint32_t qtd_fluxos = qtd_nos/2;
  
   NodeContainer nos;
   nos.Create(qtd_nos);

   NetDeviceContainer dispositivos; // utilizado para instanciar um dispositivo de rede, adicionar um endere�o MAC e uma fila ao dispositivo e instal�-lo no n�
   MobilityHelper mobility;//utilizado para atribuir posi��es e modelos de mobilidade a n�s.
  
  //Ptr<ListPositionAllocator> Alocacao = CreateObject<ListPositionAllocator>();//Alocar posi��es de uma lista determinada pelo usu�rio.
  
  //Fazendo as distribui��es dos n�s na �rea e utilizando o modelo
  //for(uint32_t i = 0; i < qtd_nos; i++)
    //  Alocacao->Add (Vector(i,posicionamento_nos[i][0],posicionamento_nos[i][1]));
      
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  "Bounds", RectangleValue (Rectangle (-350, 350, -350, 350)));

  //mobility.SetPositionAllocator (Alocacao); //Definir a alocacao de posi��o que ser� usado para alocar a posi��o inicial de cada n� inicializado durante MobilityModel :: Install.

  //Queremos que o ponto de acesso permane�a em uma posi��o fixa durante a simula��o.
   mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

   //Instalando as configura��es nos n�s
   mobility.Install (nos);
   NqosWifiMacHelper wifi_mac = NqosWifiMacHelper::Default (); //criar camadas MAC n�o habilitadas para QoS para um ns3 :: WifiNetDevice
   wifi_mac.SetType ("ns3::AdhocWifiMac");
   
   
   //Construindo os dispositivos Wifi e o canal de interliga��o entre esses n�s. Configurando os assistentes PHY e de canal:
   
   //Canal da camada f�sica
   YansWifiChannelHelper canal_wifi = YansWifiChannelHelper::Default ();
   
   //Modelo da Camada F�sica
   YansWifiPhyHelper wifi_phy = YansWifiPhyHelper::Default ();

   // Utilizando configura��es sugeridas no trabalho
   canal_wifi.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
   canal_wifi.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                                   "Exponent", DoubleValue(2.7),
                                   "ReferenceLoss", DoubleValue(1),
                                   "ReferenceDistance", DoubleValue(150));

    wifi_phy.Set ("TxPowerStart", DoubleValue(22.0));
    wifi_phy.Set ("TxPowerEnd", DoubleValue(22.0));
    wifi_phy.Set ("TxPowerLevels", UintegerValue(1));

    wifi_phy.Set ("EnergyDetectionThreshold", DoubleValue(-96.0));
    wifi_phy.Set ("CcaMode1Threshold", DoubleValue(-99.0));

   //Criando o canal apos configura��es
   wifi_phy.SetChannel (canal_wifi.Create ());
   WifiHelper wifi;
   wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  //  Setando a taxa de transmissao:
   wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
               "DataMode", StringValue("DsssRate11Mbps"),
               "ControlMode", StringValue("DsssRate11Mbps"),
               "NonUnicastMode",StringValue("DsssRate11Mbps"));

   //Instalando as configura��es do wifi
    dispositivos = wifi.Install (wifi_phy, wifi_mac, nos);
    
  Simulator::Stop (Seconds (25.0));
  Simulator::Run (); 
  
  printf("Modelo de mobilidade sendo executado com sucesso.\n");
  
  Simulator::Destroy ();

}
