// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "caffe2/core/net.h"

namespace caffe2 {

// We currently only try to convert a fixed set of operators that handle a subset of a full
// CNN. We also only run when MPSCNN is available, provides a speedup.
// On failure, returns false. On success, returns true, and sets the MPSCNN net in the output
// parameter.

bool tryConvertToMPSCNN(const NetDef& initNet, const NetDef& predictNet, NetDef* mpscnnPredictNet);

// Exposed for testing.
NetDef annotateDefWithReadCounts(const NetDef& net);
NetDef rewriteForMetal(const NetDef& net);
NetDef runMPSCNNFusion(const NetDef& net);
void dumpDef(const NetDef& d);
void mpscnnRecordExecutionFinish();
}
