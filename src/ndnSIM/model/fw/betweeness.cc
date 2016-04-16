/*
 * betweeness.cc
 *
 *  Created on: Apr 16, 2016
 *      Author: chris
 */

#include "betweeness.h"

#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndn-content-store.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

#include "ns3/assert.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"
#include "ns3/string.h"

#include <boost/ref.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/tuple/tuple.hpp>
namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.fw.Betweeness");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (Betweeness);

TypeId Betweeness::GetTypeId ()
{
	static TypeId tid = TypeId ("ns3::ndn::fw::Betweeness")
	    .SetGroupName ("Ndn")
	    .SetParent <ForwardingStrategy> ()
		.AddConstructor <Betweeness> ();
	return tid;
}

Betweeness::Betweeness ()
{

}

void Betweeness::OnInterest(Ptr<Face> inFace, Ptr<Interest> interest)
{


	  m_inInterests (interest, inFace);

	  Ptr<pit::Entry> pitEntry = m_pit->Lookup (*interest);
	  bool similarInterest = true;
	  if (pitEntry == 0)
	    {
	      similarInterest = false;
	      pitEntry = m_pit->Create (interest);
	      if (pitEntry != 0)
	        {
	          DidCreatePitEntry (inFace, interest, pitEntry);
	        }
	      else
	        {
	          FailedToCreatePitEntry (inFace, interest);
	          return;
	        }
	    }

	  bool isDuplicated = true;
	  if (!pitEntry->IsNonceSeen (interest->GetNonce ()))
	    {
	      pitEntry->AddSeenNonce (interest->GetNonce ());
	      isDuplicated = false;
	    }

	  if (isDuplicated)
	    {
	      DidReceiveDuplicateInterest (inFace, interest, pitEntry);
	      return;
	    }

	  Ptr<Data> contentObject;
	  contentObject = m_contentStore->Lookup (interest);
	  if (contentObject != 0)
	    {
		  // This section checks to see if hopCountTag exists, if it does it adds this tag to the payload of Data from Interest
		  // We can make use of this hop count tag to measure the delay
	      FwHopCountTag hopCountTag;
	      if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
	        {
	          contentObject->GetPayload ()->AddPacketTag (hopCountTag);
	        }

	      pitEntry->AddIncoming (inFace/*, Seconds (1.0)*/);

	      /*
	       * Transfer MaxBetweeness value to header of Data packet from interest packet
	       */
	      uint8_t maxBetweeness = interest->GetMaxBetweeness();
	      contentObject->SetMaxBetweeness(maxBetweeness);
	      NS_LOG_INFO("Cache hit, transferring MaxBetweeness to Data packet with value: " << maxBetweeness);

	      // Do data plane performance measurements
	      WillSatisfyPendingInterest (0, pitEntry);

	      // Actually satisfy pending interest
	      SatisfyPendingInterest (0, contentObject, pitEntry);
	      return;
	    }

	  if (similarInterest && ShouldSuppressIncomingInterest (inFace, interest, pitEntry))
	    {
	      pitEntry->AddIncoming (inFace/*, interest->GetInterestLifetime ()*/);
	      // update PIT entry lifetime
	      pitEntry->UpdateLifetime (interest->GetInterestLifetime ());

	      // Suppress this interest if we're still expecting data from some other face
	      NS_LOG_DEBUG ("Suppress interests");
	      m_dropInterests (interest, inFace);

	      DidSuppressSimilarInterest (inFace, interest, pitEntry);
	      return;
	    }

	  if (similarInterest)
	    {
	      DidForwardSimilarInterest (inFace, interest, pitEntry);
	    }

	  uint32_t node_id = this->GetObject<Node>()->GetId();
	  uint8_t betweeness = this->GetObject<Node>()->GetBetweeness();
	  NS_LOG_INFO("Received an Interest packet at Node: " << node_id);
	  NS_LOG_INFO("Betweeness at node " << node_id << " is " << std::to_string(betweeness));

	  NS_LOG_INFO("Betweeness of Interest before update: " << std::to_string(interest->GetMaxBetweeness()));

	  interest->SetMaxBetweeness(betweeness);
	  NS_LOG_INFO("MaxBetweeness of Interest packet is now: " << std::to_string(interest->GetMaxBetweeness()));

	  PropagateInterest (inFace, interest, pitEntry);
}

void Betweeness::OnData(Ptr<Face> inFace, Ptr<Data> data){
	NS_LOG_FUNCTION (inFace << data->GetName ());
	m_inData (data, inFace);

	uint8_t TSB = data->GetTimeSinceBirth();
	data->SetTimeSinceBirth(TSB + 1);

	uint32_t node_id = this->GetObject<Node>()->GetId();
	uint8_t betweeness = this->GetObject<Node>()->GetBetweeness();
	uint8_t maxBetweeness = data->GetMaxBetweeness();

	NS_LOG_INFO ("Received data packet at Node " << node_id);
	NS_LOG_INFO ("Router: Received data packet with MaxBetweeness: " << std::to_string(maxBetweeness));

	  // Lookup PIT entry
	  Ptr<pit::Entry> pitEntry = m_pit->Lookup (*data);
	  if (pitEntry == 0)
	    {
	      bool cached = false;

	      if (m_cacheUnsolicitedData || (m_cacheUnsolicitedDataFromApps && (inFace->GetFlags () & Face::APPLICATION)))
	        {
	          // Optimistically add or update entry in the content store
	          cached = m_contentStore->Add (data);
	        }
	      else
	        {
	          // Drop data packet if PIT entry is not found
	          // (unsolicited data packets should not "poison" content store)

	          //drop dulicated or not requested data packet
	          m_dropData (data, inFace);
	        }

	      DidReceiveUnsolicitedData (inFace, data, cached);
	      return;
	    }
	  else
	    {
		  /*
		   * Betweeness centrality decision to cache
		   */
		  bool cached = false;
		  if (betweeness == maxBetweeness){
			  cached = m_contentStore->Add(data);
			  NS_LOG_INFO("Content was cached");
		  } else{
			  NS_LOG_INFO("Content was not cached");
		  }

		  DidReceiveSolicitedData (inFace, data, cached);
	    }

	  while (pitEntry != 0)
	    {
	      // Do data plane performance measurements
	      WillSatisfyPendingInterest (inFace, pitEntry);

	      // Actually satisfy pending interest
	      SatisfyPendingInterest (inFace, data, pitEntry);

	      // Lookup another PIT entry
	      pitEntry = m_pit->Lookup (*data);
	    }
}

bool Betweeness::DoPropagateInterest(Ptr<Face> inFace, Ptr<const Interest> interest, Ptr<pit::Entry> pitEntry)
{

	int propagatedCount = 0;

	  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
	    {
	      NS_LOG_DEBUG ("Trying " << boost::cref(metricFace));
//	      if (metricFace.GetStatus () == fib::FaceMetric::NDN_FIB_RED) // all non-read faces are in front
//	        break;

	      if (!TrySendOutInterest (inFace, metricFace.GetFace (), interest, pitEntry))
	        {
	          continue;
	        }

	      propagatedCount++;
	      break; // do only once
	    }

	  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
	  return propagatedCount > 0;
}

}}} //end of namespaces





