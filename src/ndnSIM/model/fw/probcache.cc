/*
 * probcache.cc
 *
 *  Created on: Apr 14, 2016
 *      Author: Chris Barnes
 *
 *  Probache forwarding strategy implementation.
 */

#include "probcache.h"

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

NS_LOG_COMPONENT_DEFINE ("ndn.fw.Probcache");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (Probcache);

TypeId Probcache::GetTypeId ()
{
	static TypeId tid = TypeId ("ns3::ndn::fw::Probcache")
	    .SetGroupName ("Ndn")
	    .SetParent <ForwardingStrategy> ()
		.AddConstructor <Probcache> ();
	return tid;
}

Probcache::Probcache ()
{

}

void Probcache::OnInterest(Ptr<Face> inFace, Ptr<Interest> interest)
{
	uint32_t node_id = this->GetObject<Node>()->GetId();
	NS_LOG_INFO("Received an Interest packet at Node: " << node_id);
	NS_LOG_INFO("Betweeness at node " << node_id << " is " << this->GetObject<Node>()->GetBetweeness());

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
	       * Transfer the TSI value from the Interest packet to the new Data packet
	       */
	      uint8_t tsi = interest->GetTimeSinceInception();
	      contentObject->SetTimeSinceInception(tsi);
	      NS_LOG_INFO("Cache hit, transferring TSI to Data packet with value: " << tsi);

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

		uint8_t TSI = interest->GetTimeSinceInception();
		NS_LOG_INFO("Received Interest packet with TSI: " << std::to_string(TSI));

		interest->SetTimeSinceInception(TSI+1);
		NS_LOG_INFO("Propagating Interest packet with TSI: " << std::to_string(interest->GetTimeSinceInception()));

	  PropagateInterest (inFace, interest, pitEntry);
}

void Probcache::OnData(Ptr<Face> inFace, Ptr<Data> data){
	NS_LOG_FUNCTION (inFace << data->GetName ());
	m_inData (data, inFace);

	uint8_t TSB = data->GetTimeSinceBirth();
	data->SetTimeSinceBirth(TSB + 1);

	uint32_t node_id = this->GetObject<Node>()->GetId();

	NS_LOG_INFO ("Received data packet at Node " << node_id);
	NS_LOG_INFO ("Router: Received data packet with TSI: " << std::to_string(data->GetTimeSinceInception()));
	NS_LOG_INFO ("Router: Received data packet and incremented TSB to: " << std::to_string(data->GetTimeSinceBirth()));

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
		   * Probcache decision to cache based on TSB/TSI
		   */
		  bool cached = false;
		  uint8_t tsi = data->GetTimeSinceInception();
		  uint8_t tsb = data->GetTimeSinceBirth();
		  double prob = (double)tsi / (double)tsb;
		  double rand = ((double) std::rand() / (RAND_MAX));
		  if (rand <= prob){
		      cached = m_contentStore->Add (data);
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

bool Probcache::DoPropagateInterest(Ptr<Face> inFace, Ptr<const Interest> interest, Ptr<pit::Entry> pitEntry)
{

	NS_LOG_INFO("Inside doPropagate, the TSI value is: " << std::to_string(interest->GetTimeSinceInception()));

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

