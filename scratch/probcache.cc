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
#include <string>
#include <iostream>

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
//	LogComponentEnable ("ndn.Consumer", LOG_LEVEL_INFO);
//	LogComponentEnable ("ndn.Producer", LOG_LEVEL_INFO);
//	LogComponentEnable ("ndn.fw.Probcache", LOG_LEVEL_INFO);
//	LogComponentEnable ("ndn.fw", LOG_LEVEL_INFO);
////	LogComponentEnable ("ndn.cs.ContentStore", LOG_LEVEL_INFO);
////	LogComponentEnable ("ndn.Interest", LOG_LEVEL_INFO);
////	LogComponentEnable ("ndn.wire.ndnSIM", LOG_LEVEL_INFO);
////	LogComponentEnable ("Node", LOG_LEVEL_INFO);
//	LogComponentEnable ("ndn.fw.Betweeness", LOG_LEVEL_INFO);
//	LogComponentEnable ("AnnotatedTopologyReader", LOG_LEVEL_INFO);

	// Default parameters
	std::string frequency = "60";
	std::string cachesize = "15";
	std::string protocol = "Betweeness";
	std::string mandelbrot = "0.5";
	std::string contents = "100";

	// setting default parameters for PointToPoint links and channels
	Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("1Mbps"));
	Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10ms"));
	Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("20"));

	// Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
	CommandLine cmd;
	cmd.AddValue("frequency", "Frequency of request rate", frequency);
	cmd.AddValue("cachesize", "Cache size on each node", cachesize);
	cmd.AddValue("protocol", "Sets the forwarding strategy", protocol);
	cmd.AddValue("mandelbrot", "Sets the s value for the zipf mandelbrooooo", mandelbrot);
	cmd.AddValue("contents", "Sets number of contents available at producer", contents);
	cmd.Parse (argc, argv);

	std::cout << "Running with frequency: " << frequency << std::endl;
	std::cout << "Running with cachesize: " << cachesize << std::endl;
	std::cout << "Running with protocol: " << protocol << std::endl;
	std::cout << "Running with mandelBROOOOOO!!!: " << mandelbrot << std::endl;
	std::cout << "Running with number of contents: " << contents << std::endl;


	// Use topology builder to build tree network topology
    AnnotatedTopologyReader topologyReader ("", 10);
	topologyReader.SetFileName ("src/ndnSIM/examples/topologies/tree_topology.txt");
	topologyReader.Read ();

	NodeContainer consumerNodes = topologyReader.GetConsumerNodes();
	NodeContainer producerNodes = topologyReader.GetProducerNodes();
	NodeContainer routerNodes = topologyReader.GetRouterNodes();

	 // Install caching stack on router nodes
	ndn::StackHelper ndnRouterHelper;
	if (protocol == "Betweeness"){
	  ndnRouterHelper.SetForwardingStrategy("ns3::ndn::fw::Betweeness");
	} else if (protocol == "Probcache") {
	  ndnRouterHelper.SetForwardingStrategy("ns3::ndn::fw::Probcache");
	}
	ndnRouterHelper.SetContentStore ("ns3::ndn::cs::Lru", "MaxSize", cachesize);
	ndnRouterHelper.Install(routerNodes);

	// Install noncaching stack on consumer and producer nodes
	ndn::StackHelper ndnNocacheHelper;
	if (protocol == "Betweeness"){
		ndnNocacheHelper.SetForwardingStrategy("ns3::ndn::fw::Betweeness");
	} else if (protocol == "Probcache") {
		ndnNocacheHelper.SetForwardingStrategy("ns3::ndn::fw::Probcache");
	}
	ndnNocacheHelper.SetContentStore ("ns3::ndn::cs::Nocache");
	ndnNocacheHelper.Install(consumerNodes);
	ndnNocacheHelper.Install(producerNodes);

	// Installing global routing interface on all nodes
	ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
	ndnGlobalRoutingHelper.InstallAll ();

	// Consumer setup
	ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerZipfMandelbrot");
	consumerHelper.SetPrefix ("/prefix");
	consumerHelper.SetAttribute ("Frequency", StringValue (frequency)); // 10 interests a second
	consumerHelper.SetAttribute ("NumberOfContents", StringValue (contents));
	consumerHelper.SetAttribute ("s", StringValue (mandelbrot));
	consumerHelper.SetAttribute ("q", StringValue ("0.0"));
	consumerHelper.SetAttribute ("Randomize", StringValue ("uniform"));
	consumerHelper.Install (consumerNodes);

	// Producer setup
	ndn::AppHelper producerHelper ("ns3::ndn::Producer");
	producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
	producerHelper.SetPrefix ("/prefix");
	producerHelper.Install (producerNodes);

	// Add origins, calculate and install FIBs
	ndnGlobalRoutingHelper.AddOrigins ("/prefix", producerNodes);
	ndn::GlobalRoutingHelper::CalculateRoutes ();

	Simulator::Stop (Seconds (600.0));

	/*
	* Tracer setup
	*
	* Naming conventions:
	* aggregate-protocol-freq-cachesize-mandelbrot.txt
	* appdelay-protocol-freq-cachesize-mandelbrot.txt
	* cs-protocol-freq-cachesize-mandelbrot.txt
	*/

	std::string basename = protocol + "-" + frequency + "-" + cachesize + "-" + mandelbrot + ".txt";
	ndn::L3AggregateTracer::InstallAll("simulations/agg/aggregate-" + basename, Seconds(1.0));
	ndn::AppDelayTracer::InstallAll("simulations/delay/appdelay-" + basename);
	ndn::CsTracer::InstallAll("simulations/cs/cs-" + basename, Seconds(1.0));

	Simulator::Run ();
	Simulator::Destroy ();

	return 0;
}



