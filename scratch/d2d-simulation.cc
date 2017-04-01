/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/packet.h>

#include "ns3/applications-module.h"


#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;

int main (int argc, char *argv[])
{	
  //whether to use carrier aggregation
  bool useCa = false;

  CommandLine cmd;
  cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.Parse (argc, argv);
	
  // to save a template default attribute file run it like this:
  // ./waf --command-template="%s --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Save --ns3::ConfigStore::FileFormat=RawText" --run src/lte/examples/lena-first-sim
  //
  // to load a previously created default attribute file
  // ./waf --command-template="%s --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load --ns3::ConfigStore::FileFormat=RawText" --run src/lte/examples/lena-first-sim

  /*
    Enables Printing of Packet MetaData for Packet Information
  */ 
  std::cout << "Packet Metadata Tracing Enabled" << std::endl;
  Packet::EnablePrinting(); 

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);


  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // Parse again so you can override default values from the command line
  cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.Parse (argc, argv);

  if (useCa)
   {
     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
   }

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);


  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  std::cout << "The remote Host address is: " << remoteHostAddr << std::endl;

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);


  // Uncomment to enable logging
  //  lteHelper->EnableLogComponents ();

  // Create Nodes: eNodeB and UE
  NodeContainer enbNodes;
  NodeContainer ueNodes;
  enbNodes.Create (1);
  ueNodes.Create (2);

  // Install Mobility Model
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (enbNodes);
  BuildingsHelper::Install (enbNodes);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ueNodes);
  BuildingsHelper::Install (ueNodes);

  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs;
  // Default scheduler is PF, uncomment to use RR
  //lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);


  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
    Ptr<Node> ueNode = ueNodes.Get (u);
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }
  // ueDevs holds a LTE Net Device
  // Ptr<NetDevice> ueDevice1 = ueDevs.Get(0);

  // Downcasting to LteUeNetDevice Object
  Ptr<LteUeNetDevice> ueLteDevice1 = DynamicCast<LteUeNetDevice>(ueDevs.Get(0));
  Ptr<LteUeNetDevice> ueLteDevice2 = DynamicCast<LteUeNetDevice>(ueDevs.Get(1));
  
  // std::cout << "Main Method Access to UE MTU : " << ueDevice1->GetMtu() << std::endl;


  // Attach a UE to a eNB
  lteHelper->Attach (ueDevs, enbDevs.Get (0));

  // Activate a data radio bearer
  // enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  // EpsBearer bearer (q);
  // lteHelper->ActivateDataRadioBearer (ueDevs, bearer);

  lteHelper->EnableTraces ();

  

  std::cout << "Simulator Beginning" << std::endl;



  // GtkConfigStore config;
  // config.ConfigureAttributes ();

  // Get Phy Object Stream
  Ptr<LteUePhy> phyUE1 = ueLteDevice1->GetPhy();
  std::cout << "Printing Cache Vector Device 1" << std::endl;
  phyUE1->printCacheVector();

  // Access the Cache Vector of the First UE Device
  // std::vector<Ptr<Packet>> cacheVector1 = phyUE1->getPhyCacheVector();
  // std::cout << "Cache Vector 1 Size: " << cacheVector1.size() << std::endl;

  // Get Phy Object Stream
  Ptr<LteUePhy> phyUE2 = ueLteDevice2->GetPhy();
  std::cout << "Printing Cache Vector Device 2" << std::endl;
  phyUE2->printCacheVector();

  // Establish Link Protocol Between the UE Devices
  std::cout << "Address of UE Net Device 1: " << ueLteDevice1->GetAddress() << std::endl;
  std::cout << "Address of UE Net Device 2: " << ueLteDevice2->GetAddress() << std::endl;

  // Generate Traffic using OnOffApplication Helper
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

  //normally wouldn't need a loop here but the server IP address is different
  //on each p2p subnet
  ApplicationContainer clientApps;

  uint16_t port = 50000;

  AddressValue remoteAddress (InetSocketAddress (remoteHostAddr, port));
  clientHelper.SetAttribute ("Remote", remoteAddress);
  clientApps.Add (clientHelper.Install (ueNodes.Get(1)));
  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (10.0));


  AsciiTraceHelper ascii;
  p2ph.EnableAsciiAll (ascii.CreateFileStream ("d2dtracing.tr"));
  p2ph.EnablePcapAll ("d2dtracing");
  // Access the Cache Vector of the Second UE Device
  // std::vector<Ptr<Packet>> cacheVector2 = phyUE2->getPhyCacheVector();
  // std::cout << "Cache Vector 2 Size: " << cacheVector2.size() << std::endl;
  Simulator::Stop (Seconds (10));

  Simulator::Run ();

  Simulator::Destroy ();

  std::cout << "Simulator Ended" << std::endl;
  return 0;
}
