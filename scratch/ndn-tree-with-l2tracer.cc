// ndn-simple-withl2tracer.cc

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

using namespace ns3;

int
main (int argc, char *argv[])
{
	LogComponentEnable ("ndn.Consumer", LOG_LEVEL_INFO);
		LogComponentEnable ("ndn.Producer", LOG_LEVEL_INFO);
		LogComponentEnable ("ndn.fw.Probcache", LOG_LEVEL_INFO);
//		LogComponentEnable ("ndn.fw", LOG_LEVEL_INFO);
//		LogComponentEnable ("ndn.Interest", LOG_LEVEL_INFO);
//		LogComponentEnable ("ndn.wire.ndnSIM", LOG_LEVEL_INFO);
	//	LogComponentEnable ("Node", LOG_LEVEL_INFO);

  CommandLine cmd;
  cmd.Parse (argc, argv);

  AnnotatedTopologyReader topologyReader ("", 10);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/tree_topology.txt");
  topologyReader.Read ();
  
  /****************************************************************************/
  // Install CCNx stack on all nodes
  ndn::StackHelper ccnxHelper;
  ccnxHelper.SetContentStore ("ns3::ndn::cs::Lru", "MaxSize", "1000");
  ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::Probcache");
  ccnxHelper.InstallAll ();
  /****************************************************************************/
  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ccnxGlobalRoutingHelper;
  ccnxGlobalRoutingHelper.InstallAll ();
  /****************************************************************************/
  // Getting containers for the consumer/producer
  Ptr<Node> consumer1 = Names::Find<Node> ("cons_1");
  Ptr<Node> consumer2 = Names::Find<Node> ("cons_2");
  Ptr<Node> consumer3 = Names::Find<Node> ("cons_3");
  Ptr<Node> consumer4 = Names::Find<Node> ("cons_4");
  Ptr<Node> consumer5 = Names::Find<Node> ("cons_5");
  Ptr<Node> consumer6 = Names::Find<Node> ("cons_6");
  Ptr<Node> consumer7 = Names::Find<Node> ("cons_7");
  Ptr<Node> consumer8 = Names::Find<Node> ("cons_8");
  Ptr<Node> consumer9 = Names::Find<Node> ("cons_9");
  Ptr<Node> consumer10 = Names::Find<Node> ("cons_10");
  Ptr<Node> consumer11 = Names::Find<Node> ("cons_11");
  Ptr<Node> consumer12 = Names::Find<Node> ("cons_12");
  Ptr<Node> consumer13 = Names::Find<Node> ("cons_13");
  Ptr<Node> consumer14 = Names::Find<Node> ("cons_14");
  Ptr<Node> consumer15 = Names::Find<Node> ("cons_15");
  Ptr<Node> consumer16 = Names::Find<Node> ("cons_16");

  Ptr<Node> producer1 = Names::Find<Node> ("prod_1");
  Ptr<Node> producer2 = Names::Find<Node> ("prod_2");

  /****************************************************************************/
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute ("Frequency", StringValue ("1000"));//interests per Second
  consumerHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  /****************************************************************************/
  // on the first to ninth consumer node install a Consumer application
  // that will express interests in /dst1 to /dst9 namespace
  consumerHelper.SetPrefix ("/dst9");
  consumerHelper.Install (consumer1);

  consumerHelper.SetPrefix ("/dst8");
  consumerHelper.Install (consumer2);

  consumerHelper.SetPrefix ("/dst7");
  consumerHelper.Install (consumer3);

  consumerHelper.SetPrefix ("/dst6");
  consumerHelper.Install (consumer4);

  consumerHelper.SetPrefix ("/dst5");
  consumerHelper.Install (consumer5);

  consumerHelper.SetPrefix ("/dst4");
  consumerHelper.Install (consumer6);

  consumerHelper.SetPrefix ("/dst3");
  consumerHelper.Install (consumer7);

  consumerHelper.SetPrefix ("/dst2");
  consumerHelper.Install (consumer8);

  consumerHelper.SetPrefix ("/dst1");
  consumerHelper.Install (consumer9);
  
  consumerHelper.SetPrefix ("/dst10");
  consumerHelper.Install (consumer10);

  consumerHelper.SetPrefix ("/dst11");
  consumerHelper.Install (consumer11);

  consumerHelper.SetPrefix ("/dst12");
  consumerHelper.Install (consumer12);

  consumerHelper.SetPrefix ("/dst13");
  consumerHelper.Install (consumer13);

  consumerHelper.SetPrefix ("/dst14");
  consumerHelper.Install (consumer14);

  consumerHelper.SetPrefix ("/dst15");
  consumerHelper.Install (consumer15);

  consumerHelper.SetPrefix ("/dst16");
  consumerHelper.Install (consumer16);

  /****************************************************************************/
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  /****************************************************************************/
  // Register /dst1 to /dst9 prefix with global routing controller and
  // install producer that will satisfy Interests in /dst1 to /dst9 namespace
  ccnxGlobalRoutingHelper.AddOrigins ("/dst1", producer1);
  producerHelper.SetPrefix ("/dst1");
  producerHelper.Install (producer1);

  ccnxGlobalRoutingHelper.AddOrigins ("/dst2", producer2);
  producerHelper.SetPrefix ("/dst2");
  producerHelper.Install (producer2);


  /*****************************************************************************/
  // Calculate and install FIBs
  ccnxGlobalRoutingHelper.CalculateRoutes ();

  Simulator::Stop (Seconds (10.0));

  /****************************************************************************/
  //Tracer:

  L2RateTracer::InstallAll ("drop-trace.txt", Seconds (0.5));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
