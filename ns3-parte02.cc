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

void pacoteRecebido(Ptr<Socket> socket);
static void gerarTrafego(Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval);
void escolherFluxos(NodeContainer &nos, Ipv4InterfaceContainer &interfaces, 
    float **distancia_nos, uint32_t qtd_fluxos, uint32_t qtd_nos);
void simulacao(uint32_t qtd_nos, uint32_t area, int rts, FILE **file_razao, FILE **file_vazao, int matriz_posicao_nos[][2]);

int main()
{
    srand(time(NULL));
    int i;

    FILE *file_razao, *through;
    file_razao = fopen("Razao", "wt");
    through = fopen("Throughput", "wt");

    int area = 800;
    int qtd_nos = 50;
    int matriz_posicao_nos[qtd_nos][2];

    for (i = 0; i < qtd_nos; i++)
    {
        matriz_posicao_nos[i][0] = rand() % area;
        matriz_posicao_nos[i][1] = rand() % area;
    }
    // Teste sem RTS/CTS
    simulacao(qtd_nos, area, 1, &file_razao, &through, matriz_posicao_nos);
    // Teste com RTS/CTS
    simulacao(qtd_nos, area, 0, &file_razao, &through, matriz_posicao_nos);

    return 0;
}

void simulacao(uint32_t qtd_nos, uint32_t area, int rts, FILE **file_razao, FILE **file_vazao, int matriz_posicao_nos[][2])
{
    uint32_t i,j; //iterators
    uint32_t qtd_fluxos = qtd_nos / 2;

    NodeContainer nos;
    nos.Create(qtd_nos);

    NetDeviceContainer dispositivos;// utilizado para instanciar um dispositivo de rede, 
                                    //adicionar um endereço MAC e uma fila ao dispositivo e instalá-lo no nó
    MobilityHelper mobility;//utilizado para atribuir posições e modelos de mobilidade a nós.

    //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    //"Bounds", RectangleValue (Rectangle (-350, 350, -350, 350)));

    float **distancia_nos = (float**)malloc(sizeof(float*)*qtd_nos);
    for(i=0;i<qtd_nos;i++)
        distancia_nos[i] = (float*)malloc(sizeof(float)*qtd_nos);

    //calculando distancia entre cada no
    for (i = 0; i < qtd_nos; i++)
        for (j = 0; j < qtd_nos; j++)
            distancia_nos[i][j] = sqrt((matriz_posicao_nos[i][0] - matriz_posicao_nos[j][0]) * (matriz_posicao_nos[i][0] - matriz_posicao_nos[j][0]) + (matriz_posicao_nos[i][1] - matriz_posicao_nos[j][1]) * (matriz_posicao_nos[i][1] - matriz_posicao_nos[j][1]));
    
    //alocar posições de uma lista determinada pelo usuário.
    Ptr<ListPositionAllocator> Alocacao_inicial = CreateObject<ListPositionAllocator>();
    
    //distribuindo os nos na area e usando o modelo
    for (uint32_t i = 0; i < qtd_nos; i++)
        Alocacao_inicial->Add(Vector(i, matriz_posicao_nos[i][0], matriz_posicao_nos[i][1]));

    mobility.SetPositionAllocator(Alocacao_inicial);
    //definindo que não mudara as posições dos nos, ate que redefina explicitamente
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    //instalando os nos
    mobility.Install(nos);

    //criar camadas MAC não habilitadas para QoS para um ns3 :: WifiNetDevice
    NqosWifiMacHelper wifi_mac = NqosWifiMacHelper::Default();
    wifi_mac.SetType("ns3::AdhocWifiMac");

    //modelo da Camada fisica
    YansWifiPhyHelper wifi_phy = YansWifiPhyHelper::Default();
    
    //construindo os dispositivos Wifi e o canal de interligação entre esses nós. Configurando os assistentes PHY e de canal:
    
    //Canal da camada física
    YansWifiChannelHelper canal_wifi = YansWifiChannelHelper::Default();

    //utilizando configuracoes sugeridas no trabalho
    canal_wifi.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    canal_wifi.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                  "Exponent", DoubleValue(2.7),
                                  "ReferenceLoss", DoubleValue(1),
                                  "ReferenceDistance", DoubleValue(150));

    wifi_phy.Set("TxPowerStart", DoubleValue(22.0));
    wifi_phy.Set("TxPowerEnd", DoubleValue(22.0));
    wifi_phy.Set("TxPowerLevels", UintegerValue(1));

    wifi_phy.Set("EnergyDetectionThreshold", DoubleValue(-96.0));
    wifi_phy.Set("CcaMode1Threshold", DoubleValue(-99.0));

    //Depois de configurado, estamos criando o canal
    wifi_phy.SetChannel(canal_wifi.Create());
    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

    //  Setando a taxa de transmissao:
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue("DsssRate11Mbps"),
                                 "ControlMode", StringValue("DsssRate11Mbps"),
                                 "NonUnicastMode", StringValue("DsssRate11Mbps"));

    //Instalando as configurações do wifi
    dispositivos = wifi.Install(wifi_phy, wifi_mac, nos);

    //CRIANDO PILHA DE INTERNET

    //Criando a variável internet
    InternetStackHelper internet;

    //Instalando as configurações na internet
    internet.Install(nos);

    //Estilo de roteamento
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    //Definindo os endereços do ipv4 e o (intervalo de endereços possíveis)
    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.0.0.0");
    //Mantém um vetor de pares, Ptr<Ipv4> e índice, para a interface
    Ipv4InterfaceContainer interfaces;

    //Alocar para um endereço para cada dispositivo
    interfaces = address.Assign(dispositivos);

    //Instalando o FlowMonitorHelper em todos os nós
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    //escolha de fluxos
    escolherFluxos(nos, interfaces, distancia_nos, qtd_fluxos, qtd_nos);

    //Simulacao
    Simulator::Stop(Seconds(40.0));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    double Razao = 0;
    double vazao = 0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Razao += 1.0 * iter->second.rxBytes / iter->second.txBytes;
        vazao += 8.0 * iter->second.rxBytes / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1024;
    }

    if (rts == 1)
    {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(1400));
        fprintf(*file_razao, "\nSem RTS/CTS\n");
    }
    else
    {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(2500));
        fprintf(*file_razao, "\nCom RTS/CTS\n");
    }

    fprintf(*file_vazao, "%lf\n", vazao / qtd_fluxos); 
    fprintf(*file_razao, "%lf\n", Razao / qtd_fluxos);

    for(i=0;i<qtd_nos;i++)
        free(distancia_nos[i]);
    free(distancia_nos);

    Simulator::Destroy();
}

void escolherFluxos(NodeContainer &nos, Ipv4InterfaceContainer &interfaces, float **distancia_nos, uint32_t qtd_fluxos, uint32_t qtd_nos)
{
    uint32_t i; //iterator
    uint32_t fluxo;
    uint32_t receptor = 0;
    uint32_t remetente;

    //vetor que servira para marcar quem ja foi escolhido
    bool no_marcado[qtd_nos];

    TypeId typeId = TypeId::LookupByName("ns3::UdpSocketFactory");

    //definindo o valor inicial
    for (i = 0; i < qtd_nos; i++)
        no_marcado[i] = false;

    for (fluxo = 0; fluxo < qtd_fluxos; fluxo++)
    {
        remetente = rand() % qtd_nos;
        while (no_marcado[remetente] != false)
            remetente = rand() % qtd_nos;

        no_marcado[remetente] = true;

        float distancia = 990;

        //escolhendo receptor que ainda nao foi escolhido e que seja o mais proximo do remetente
        for (i = 0; i < qtd_nos; i++)
        {
            if (no_marcado[i] == false && distancia_nos[remetente][i] < distancia)
            {
                distancia = distancia_nos[remetente][i];
                receptor = i;
            }
        }

        no_marcado[receptor] = true;

        //cria o socket receptor
        Ptr<Socket> recvSink = Socket::CreateSocket(nos.Get(receptor), typeId);
        //recebe um endereco
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 2001);
        //vincula o socket ao endereco
        recvSink->Bind(local);
        //configura procedimento a ser realizado quando um pacote e recebido com sucesso
        recvSink->SetRecvCallback(MakeCallback(&pacoteRecebido));

        //cria o socket remetente
        Ptr<Socket> source = Socket::CreateSocket(nos.Get(remetente), typeId);
        //cria remote para armazenar o endereco do receptor
        InetSocketAddress remote = InetSocketAddress(interfaces.GetAddress(receptor), 2001);

        source->Connect(remote);

        Simulator::ScheduleWithContext(source->GetNode()->GetId(),
                                       Seconds(1.0), &gerarTrafego,
                                       source, 2200, 1, Seconds(1.0));
    }
}

//Se o pacote for recebido com sucesso, essa funcao sera executada
void pacoteRecebido(Ptr<Socket> socket)
{
    NS_LOG_UNCOND("No: " << socket->GetNode()->GetId() << "\tRecebido com sucesso");
}

//gerar o trafego
static void gerarTrafego(Ptr<Socket> socket, uint32_t pktSize,
                         uint32_t pktCount, Time pktInterval)
{
    if (pktCount > 0)
    {
        socket->Send(Create<Packet>(pktSize));
        Simulator::Schedule(pktInterval, &gerarTrafego,
                            socket, pktSize, pktCount - 1, pktInterval);
    }
    else
        socket->Close();
}