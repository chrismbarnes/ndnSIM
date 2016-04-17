/*
 * probcache.cc
 *
 *  Created on: Apr 11, 2016
 *      Author: chris
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

using namespace ns3;

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

int
main (int argc, char *argv[])
{
	LogComponentEnable ("ndn.Consumer", LOG_LEVEL_INFO);
	LogComponentEnable ("ndn.Producer", LOG_LEVEL_INFO);
	LogComponentEnable ("ndn.fw.Probcache", LOG_LEVEL_INFO);
	LogComponentEnable ("ndn.fw", LOG_LEVEL_INFO);
//	LogComponentEnable ("ndn.cs.ContentStore", LOG_LEVEL_INFO);
//	LogComponentEnable ("ndn.Interest", LOG_LEVEL_INFO);
//	LogComponentEnable ("ndn.wire.ndnSIM", LOG_LEVEL_INFO);
//	LogComponentEnable ("Node", LOG_LEVEL_INFO);
	LogComponentEnable ("ndn.fw.Betweeness", LOG_LEVEL_INFO);


  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("1Mbps"));
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10ms"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("20"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create (4);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install (nodes.Get (0), nodes.Get (1));
  p2p.Install (nodes.Get (1), nodes.Get (2));
  p2p.Install (nodes.Get (2), nodes.Get (3));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy("ns3::ndn::fw::Betweeness");
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.InstallAll ();

  // Print betweeness (test)
  nodes.Get (0)->SetBetweeness(0);
  nodes.Get (1)->SetBetweeness(4);
  nodes.Get (2)->SetBetweeness(4);
  nodes.Get (3)->SetBetweeness(0);


  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerZipfMandelbrot");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix ("/prefix");
  consumerHelper.SetAttribute ("Frequency", StringValue ("100")); // 10 interests a second
  consumerHelper.SetAttribute ("NumberOfContents", StringValue ("100"));
  consumerHelper.Install (nodes.Get (0)); // first node

  // Producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/prefix");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.Install (nodes.Get (3)); // last node

  Simulator::Stop (Seconds (4.0));

  ndn::L3AggregateTracer::InstallAll("test-aggregate-trace.txt", Seconds(1.0));
  ndn::AppDelayTracer::InstallAll("test-app-delay-trace.txt");
  ndn::CsTracer::InstallAll("test-cs-trace.txt", Seconds(1.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}



