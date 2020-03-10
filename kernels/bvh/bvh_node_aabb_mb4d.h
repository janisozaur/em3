// ======================================================================== //
// Copyright 2009-2020 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "bvh_node_aabb_mb.h"

namespace embree
{
  /*! Aligned 4D Motion Blur Node */
  template<typename NodeRef, int N>
    struct AlignedNodeMB4D_t : public AlignedNodeMB_t<NodeRef, N>
  {
    using BaseNode_t<NodeRef,N>::children;
    using AlignedNodeMB_t<NodeRef,N>::set;

    typedef BVHNodeRecord<NodeRef>     NodeRecord;
    typedef BVHNodeRecordMB<NodeRef>   NodeRecordMB;
    typedef BVHNodeRecordMB4D<NodeRef> NodeRecordMB4D;
    
    struct Create
    {
      template<typename BuildRecord>
      __forceinline NodeRef operator() (BuildRecord*, const size_t, const FastAllocator::CachedAllocator& alloc, bool hasTimeSplits = true) const
      {
        if (hasTimeSplits)
        {
          AlignedNodeMB4D_t* node = (AlignedNodeMB4D_t*) alloc.malloc0(sizeof(AlignedNodeMB4D_t),NodeRef::byteNodeAlignment); node->clear();
          return NodeRef::encodeNode(node);
        }
        else
        {
          AlignedNodeMB_t<NodeRef,N>* node = (AlignedNodeMB_t<NodeRef,N>*) alloc.malloc0(sizeof(AlignedNodeMB_t<NodeRef,N>),NodeRef::byteNodeAlignment); node->clear();
          return NodeRef::encodeNode(node);
        }
      }
    };

    struct Set
    {
      template<typename BuildRecord>
      __forceinline void operator() (const BuildRecord&, const BuildRecord*, NodeRef ref, NodeRecordMB4D* children, const size_t num) const
      {
        if (likely(ref.isAlignedNodeMB())) {
          for (size_t i=0; i<num; i++)
            ref.alignedNodeMB()->set(i, children[i]);
        } else {
          for (size_t i=0; i<num; i++)
            ref.alignedNodeMB4D()->set(i, children[i]);
        }
      }
    };

    /*! Clears the node. */
    __forceinline void clear()  {
      lower_t = vfloat<N>(pos_inf);
      upper_t = vfloat<N>(neg_inf);
      AlignedNodeMB_t<NodeRef,N>::clear();
    }
    
    /*! Sets bounding box of child. */
    __forceinline void setBounds(size_t i, const LBBox3fa& bounds, const BBox1f& tbounds)
    {
      AlignedNodeMB_t<NodeRef,N>::setBounds(i, bounds.global(tbounds));
      lower_t[i] = tbounds.lower;
      upper_t[i] = tbounds.upper == 1.0f ? 1.0f+float(ulp) : tbounds.upper;
    }
    
    /*! Sets bounding box and ID of child. */
    __forceinline void set(size_t i, const NodeRecordMB4D& child) {
      AlignedNodeMB_t<NodeRef,N>::setRef(i,child.ref);
      setBounds(i, child.lbounds, child.dt);
    }
    
    /*! Returns the expected surface area when randomly sampling the time. */
    __forceinline float expectedHalfArea(size_t i) const {
      return AlignedNodeMB_t<NodeRef,N>::lbounds(i).expectedHalfArea(timeRange(i));
    }
    
    /*! returns time range for specified child */
    __forceinline BBox1f timeRange(size_t i) const {
      return BBox1f(lower_t[i],upper_t[i]);
    }
    
    /*! stream output operator */
    friend embree_ostream operator<<(embree_ostream cout, const AlignedNodeMB4D_t& n) 
    {
      cout << "AlignedNodeMB4D {" << embree_endl;
      for (size_t i=0; i<N; i++) 
      {
        const BBox3fa b0 = n.bounds0(i);
        const BBox3fa b1 = n.bounds1(i);
        cout << "  child" << i << " { " << embree_endl;
        cout << "    bounds0 = " << lerp(b0,b1,n.lower_t[i]) << ", " << embree_endl;
        cout << "    bounds1 = " << lerp(b0,b1,n.upper_t[i]) << ", " << embree_endl;
        cout << "    time_bounds = " << n.lower_t[i] << ", " << n.upper_t[i] << embree_endl;
        cout << "  }";
      }
      cout << "}";
      return cout;
    }
    
  public:
    vfloat<N> lower_t;        //!< time dimension of lower bounds of all N children
    vfloat<N> upper_t;        //!< time dimension of upper bounds of all N children
  };
}