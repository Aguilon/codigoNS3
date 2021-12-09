#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/buildings-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
int main (int argc, char *argv[])
{
 // Parametros da simulação
 bool downlink = true;
 bool uplink = true;
 bool tcp = true;
 bool udp = true;
 uint16_t simTime = 20;
 CommandLine cmd;
 cmd.AddValue ("simTime", "Simulation time (s)", simTime);
 cmd.AddValue ("down", "Downlink traffic", downlink);
 cmd.AddValue ("up", "Uplink traffic", uplink);
 cmd.AddValue ("tcp", "TCP traffic", ping);
 cmd.AddValue ("udp", "UDP traffic", voip);
 cmd.Parse (argc, argv);
 
 // Parametros pro tráfego udp
 uint32_t MaxPacketSize = 1024;
 Time interPacketInterval = Seconds (0.0001);
 uint32_t maxPacketCount = 2147483647;
 
 // Configuring dl and up transmission bandwidth in number of RBs
 Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (100));
 Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (100));
 
 // Create lte and epc helpers
 Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
 Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
 lteHelper->SetEpcHelper (epcHelper);
 Ptr<Node> pgw = epcHelper->GetPgwNode (); // LTE EPC gateway
 
 // Create a single RemoteHost
 NodeContainer remoteHostContainer;
 remoteHostContainer.Create (1);
 Ptr<Node> remoteHost = remoteHostContainer.Get (0);
 InternetStackHelper internet;
 internet.Install (remoteHostContainer);
 
 // Create the Internet
 PointToPointHelper p2ph;
 p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gbps")));
 p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
 p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
 NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
 Ipv4AddressHelper ipv4h;
 ipv4h.SetBase ("10.1.0.0", "255.255.0.0");
 Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
 Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
 Ipv4StaticRoutingHelper ipv4RoutingHelper;
 Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
 ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
 remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"),
 Ipv4Mask ("255.0.0.0"), 1);
 
 // Create Nodes: eNB and UE
 NodeContainer enbNodes;
 NodeContainer ueNodes;
 enbNodes.Create (1);
 ueNodes.Create (1);
 Ptr<Node> ueNode = ueNodes.Get (0);
 
 // Install Mobility Model
 MobilityHelper mobility;
 mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
 mobility.Install (enbNodes);
 BuildingsHelper::Install (enbNodes);
 mobility.Install (ueNodes);
 BuildingsHelper::Install (ueNodes);
 
 // Create Devices and install them in the Nodes (eNB and UE)
 NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNodes);
 NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);
 
 // Install the IP stack on the UE
 internet.Install (ueNodes);
 Ipv4InterfaceContainer ueIpIfaces = epcHelper->AssignUeIpv4Address
 (NetDeviceContainer (ueDevs));
 
 // Set the default gateway for the UE
 Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting
 (ueNode->GetObject<Ipv4> ());
 ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
 
 // Attach the UE to a eNB
 lteHelper->Attach (ueDevs, enbDevs.Get (0));
 
 // Install a TCP application
 ApplicationContainer clientApps;
 ApplicationContainer serverApps;

 uint16_t dlPort = 10000;
 uint16_t ulPort = 20000;
 if (tcp)
 {
	 if (downlink)
	 {
	 BulkSendHelper dlClientHelper ("ns3::TcpSocketFactory",
	 InetSocketAddress (ueIpIfaces.GetAddress (0), dlPort));
	 dlClientHelper.SetAttribute ("MaxBytes", UintegerValue (0));
	 clientApps.Add (dlClientHelper.Install (remoteHost));
	 PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory",
	 InetSocketAddress (Ipv4Address::GetAny (), dlPort));
	 serverApps.Add (dlPacketSinkHelper.Install (ueNode));
	 }
	 if (uplink)
	 {
	 BulkSendHelper ulClientHelper ("ns3::TcpSocketFactory",
	 InetSocketAddress (remoteHostAddr, ulPort));
	 ulClientHelper.SetAttribute ("MaxBytes", UintegerValue (0));
	 clientApps.Add (ulClientHelper.Install (ueNode));
	 PacketSinkHelper ulPacketSinkHelper ("ns3::TcpSocketFactory",
	 InetSocketAddress (Ipv4Address::GetAny (), ulPort));
	 serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
	 }
 }
 
if (udp)
 {
	 if (downlink)
	 {
	 UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (0), dlPort);
	 dlClientHelper.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
	 dlClientHelper.SetAttribute ("Interval", TimeValue (interPacketInterval));
	 dlClientHelper.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
	 clientApps.Add (dlClientHelper.Install (remoteHost));
	 PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
	 InetSocketAddress (Ipv4Address::GetAny (), dlPort));
	 serverApps.Add (dlPacketSinkHelper.Install (ueNode));
	 }
	 if (uplink)
	 {
	 UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
	 ulClientHelper.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
	 ulClientHelper.SetAttribute ("Interval", TimeValue (interPacketInterval));
	 ulClientHelper.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
	 clientApps.Add (ulClientHelper.Install (ueNode));
	 PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory",
	 InetSocketAddress (Ipv4Address::GetAny (), ulPort));
	 serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
	 }
 }
 serverApps.Start (Seconds (0));
 clientApps.Start (Seconds (1));
 
 // Output traces
 FlowMonitorHelper flowmonHelper;
 Ptr<FlowMonitor> flowmon = flowmonHelper.Install (remoteHost);
 p2ph.EnablePcap ("mazzoni", remoteHostContainer);
 Simulator::Stop (Seconds (simTime));
 Simulator::Run ();
 
 for (int i = 0; i < serverApps.GetN (); i++)
 {
 Ptr<PacketSink> sink = DynamicCast<PacketSink> (sinkApps.Get (i));
 std::cout << "Total Bytes Received: " << sink->GetTotalRx () << " KBytes/sec: "
 << (sink->GetTotalRx ()/1020)/(simTime-1) << std::endl;
 }
 flowmon->SerializeToXmlFile ("estatisticas.xml", false, false);
 Simulator::Destroy ();
 return 0;
}
