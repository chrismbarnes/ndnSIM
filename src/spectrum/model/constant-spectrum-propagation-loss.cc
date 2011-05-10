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

#include "ns3/log.h"

#include "ns3/constant-spectrum-propagation-loss.h"
#include "ns3/double.h"


NS_LOG_COMPONENT_DEFINE ("ConstantSpectrumPropagationLossModel");

namespace ns3 {


NS_OBJECT_ENSURE_REGISTERED (ConstantSpectrumPropagationLossModel);

ConstantSpectrumPropagationLossModel::ConstantSpectrumPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
}

ConstantSpectrumPropagationLossModel::~ConstantSpectrumPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
ConstantSpectrumPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConstantSpectrumPropagationLossModel")
    .SetParent<SpectrumPropagationLossModel> ()
    .AddConstructor<ConstantSpectrumPropagationLossModel> ()
    .AddAttribute ("Loss",
                   "Path loss (dB) between transmitter and receiver",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&ConstantSpectrumPropagationLossModel::m_loss),
                   MakeDoubleChecker<double> ())
    ;
  return tid;
}


Ptr<SpectrumValue>
ConstantSpectrumPropagationLossModel::DoCalcRxPowerSpectralDensity (Ptr<const SpectrumValue> txPsd,
                                                                    Ptr<const MobilityModel> a,
                                                                    Ptr<const MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);

  Ptr<SpectrumValue> rxPsd = Copy<SpectrumValue> (txPsd);
  Values::iterator vit = rxPsd->ValuesBegin ();
  Bands::const_iterator fit = rxPsd->ConstBandsBegin ();

//   NS_LOG_INFO ("Loss = " << m_loss);
  while (vit != rxPsd->ValuesEnd ())
    {
      NS_ASSERT (fit != rxPsd->ConstBandsEnd ());
//       NS_LOG_INFO ("Ptx = " << *vit);
      *vit /= m_loss; // Prx = Ptx / loss
//       NS_LOG_INFO ("Prx = " << *vit);
      ++vit;
      ++fit;
    }
  return rxPsd;
}


}  // namespace ns3