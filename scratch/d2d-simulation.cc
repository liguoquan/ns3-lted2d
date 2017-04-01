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

  /*
    Enables Printing of Packet MetaData for Packet Information
  */ 
  std::cout << "Packet Metadata Tracing Enabled" << std::endl;
  Packet::EnablePrinting(); 

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

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

  // ueDevs holds a LTE Net Device
  // Ptr<NetDevice> ueDevice1 = ueDevs.Get(0);

  // Downcasting to LteUeNetDevice Object
  Ptr<LteUeNetDevice> ueLteDevice1 = DynamicCast<LteUeNetDevice>(ueDevs.Get(0));
  Ptr<LteUeNetDevice> ueLteDevice2 = DynamicCast<LteUeNetDevice>(ueDevs.Get(1));
  
  // std::cout << "Main Method Access to UE MTU : " << ueDevice1->GetMtu() << std::endl;


  // Attach a UE to a eNB
  lteHelper->Attach (ueDevs, enbDevs.Get (0));

  // Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  lteHelper->ActivateDataRadioBearer (ueDevs, bearer);
  lteHelper->EnableTraces ();

  Simulator::Stop (Seconds (0.05));

  std::cout << "Simulator Beginning" << std::endl;

  Simulator::Run ();

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


  // Generate VBR Traffic using OnOffApplication Helper

  // OnOffHelper onoff ("ns3::UdpSocketFactory", InetSocketAddress(serverAddr,9));
  // onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  // onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  // onoff.SetConstantRate(DataRate("4Mbps"), packetSize);

  // Access the Cache Vector of the Second UE Device
  // std::vector<Ptr<Packet>> cacheVector2 = phyUE2->getPhyCacheVector();
  // std::cout << "Cache Vector 2 Size: " << cacheVector2.size() << std::endl;

  Simulator::Destroy ();

  std::cout << "Simulator Ended" << std::endl;
  return 0;
}
