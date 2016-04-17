/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2012 University of California, Los Angeles
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */
// ndn-grid-topo-plugin.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

using namespace ns3;

/**
 * This scenario simulates a grid topology (using topology reader module)
 *
 * (consumer) -- ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) -- (producer)
 *
 * All links are 1Mbps with propagation 10ms delay. 
 *
 * FIB is populated using NdnGlobalRoutingHelper.
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-grid-topo-plugin
 */

int
main (int argc, char *argv[])
{
	LogComponentEnable ("ndn.Consumer", LOG_LEVEL_INFO);
	LogComponentEnable ("ndn.Producer", LOG_LEVEL_INFO);
	LogComponentEnable ("ndn.fw.Probcache", LOG_LEVEL_INFO);
	LogComponentEnable ("ndn.fw.Betweeness", LOG_LEVEL_INFO);

  CommandLine cmd;
  cmd.Parse (argc, argv);

  AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/grid_topology.txt");
  topologyReader.Read ();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::Betweeness");
  ndnHelper.InstallAll ();

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  // Getting containers for the consumer/producer
  Ptr<Node> producer1 = Names::Find<Node> ("prod_1");
  Ptr<Node> producer2 = Names::Find<Node> ("prod_2");
  Ptr<Node> producer3 = Names::Find<Node> ("prod_3");

  NodeContainer consumerNodes;

  consumerNodes.Add (Names::Find<Node> ("cons_1"));
  consumerNodes.Add (Names::Find<Node> ("cons_2"));
  consumerNodes.Add (Names::Find<Node> ("cons_3"));
  consumerNodes.Add (Names::Find<Node> ("cons_4"));
  consumerNodes.Add (Names::Find<Node> ("cons_5"));
  consumerNodes.Add (Names::Find<Node> ("cons_6"));
  consumerNodes.Add (Names::Find<Node> ("cons_7"));


  // Install NDN applications
  std::string prefix = "/prefix";
//
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix (prefix);
  consumerHelper.SetAttribute ("Frequency", StringValue ("100")); // 100 interests a second
  consumerHelper.Install (consumerNodes);

  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetPrefix (prefix);
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.Install (producer1);
  producerHelper.Install (producer2);
  producerHelper.Install (producer3);


  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins (prefix, producer1);
  ndnGlobalRoutingHelper.AddOrigins (prefix, producer2);
  ndnGlobalRoutingHelper.AddOrigins (prefix, producer3);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes ();

  Simulator::Stop (Seconds (1.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
