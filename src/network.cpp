/*CppFile****************************************************************

  FileName    [network.cpp]

  Synopsis    []

  Author      [Chang Meng]

  Date        [09/08/2018 15:22:29]

***********************************************************************/

#include "network.h"

namespace user
{

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
uint64_t * network_t::accurateValue = nullptr;
uint64_t * network_t::currentValue = nullptr;
uint64_t * network_t::tempValue = nullptr;
abc::Abc_Obj_t ** network_t::pInvs = nullptr;
float network_t::originalDelay = 0;
float network_t::originalArea = 0;
float network_t::invDelay = 0;
int network_t::errorUpperBound = 0;
int network_t::simulationFrameNum = 0;
int network_t::frameBaseNum = 0;
int network_t::truncShift = 0;

network_t::network_t(abc::Abc_Ntk_t * p_ntk, int simulation_frame_num, const char * file_name, float error_rate)
{
	abc::Abc_Obj_t * pObj;
	int i;
    if (file_name != nullptr) {
        simulationFrameNum = simulation_frame_num;
        frameBaseNum = ((simulationFrameNum - 1) >> 6) + 1;
        truncShift = 63 - ((simulationFrameNum - 1) & 63);
    }

	// allocate memory
	gateType = (gate_t *)malloc(sizeof(gate_t) * maxObjNum);
    if (file_name != nullptr)
	    pInvs = (abc::Abc_Obj_t **)malloc(sizeof(abc::Abc_Obj_t *) * maxObjNum);
	reconvPaths = (abc::Vec_Ptr_t **)malloc(sizeof(abc::Vec_Ptr_t *) * maxObjNum);
	fringes = (abc::Vec_Ptr_t **)malloc(sizeof(abc::Vec_Ptr_t *) * maxObjNum);
    if (file_name != nullptr) { // only allocated when file_name exists
        accurateValue = (uint64_t *)malloc(sizeof(uint64_t) * maxObjNum * maxFrameNum >> 6);
        currentValue = (uint64_t *)malloc(sizeof(uint64_t) * maxObjNum * maxFrameNum >> 6);
        tempValue = (uint64_t *)malloc(sizeof(uint64_t) * maxObjNum * maxFrameNum >> 6);
    }
	CPM = (uint64_t *)malloc(sizeof(uint64_t) * maxObjNum * maxPoNum);
	PPM = (uint64_t *)malloc(sizeof(uint64_t) * maxObjNum);
	NPM = (uint64_t *)malloc(sizeof(uint64_t) * maxObjNum);
	isPoICorrect = (uint64_t *)malloc(sizeof(uint64_t) * maxPoNum);
	isPOInFOH = (uint64_t *)malloc(sizeof(uint64_t) * maxObjNum);
	isPOInFOM = (uint64_t *)malloc(sizeof(uint64_t) * maxObjNum);
	isPOInFOL = (uint64_t *)malloc(sizeof(uint64_t) * maxObjNum);
	addedER = (int *)malloc(sizeof(int) * maxObjNum * maxObjNum);
	count1Table = (int8_t *)malloc(sizeof(int8_t) * count1TableLength);
	candidates = new candidate_t[maxObjNum * maxObjNum];

	// fix network
	pNtk = Abc_NtkDup(p_ntk);
	RenewConst(pNtk);
    if (file_name != nullptr)
	    AddInverters(pNtk);

    // pre-allocate reconvergent paths
    Abc_NtkForEachNode(pNtk, pObj, i) {
        reconvPaths[pObj->Id] = abc::Vec_PtrAlloc(1000);
        fringes[pObj->Id] = abc::Vec_PtrAlloc(10);
    }

	// get gate type
	Abc_NtkForEachNode(pNtk, pObj, i)
		gateType[pObj->Id] = GetGateType(pObj);

	// get quick count table
	memset(count1Table, 0, sizeof(uint8_t) * count1TableLength);
	for (i = 0; i < count1TableLength; ++i)
		for ( uint64_t temp = (uint64_t)i; temp; temp >>= 1 )
			if (temp & (uint64_t)1)
				++count1Table[i];

	// get input vectors
    if (file_name != nullptr)
	    GetInputValue(file_name);

	// get accurate value
    if (file_name != nullptr)
	    GetAllValue(accurateValue);

	// get vital parameters
    if (file_name != nullptr) {
        errorUpperBound = error_rate * simulation_frame_num;
        originalArea = abc::Abc_NtkGetMappedArea(pNtk);
        originalDelay = abc::GetArrivalTime(pNtk);
        invDelay = abc::Mio_LibraryReadDelayInvRise( (abc::Mio_Library_t *)abc::Abc_FrameReadLibGen() );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
network_t::~network_t()
{
    abc::Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachNode(pNtk, pObj, i) {
        abc::Vec_PtrFree(reconvPaths[pObj->Id]);
        abc::Vec_PtrFree(fringes[pObj->Id]);
    }

    abc::Abc_NtkDelete(pNtk);
    free(reconvPaths);
    free(fringes);
	free(gateType);
	free(CPM);
	free(PPM);
	free(NPM);
	free(isPoICorrect);
    free(isPOInFOH);
    free(isPOInFOM);
    free(isPOInFOL);
	free(addedER);
	free(count1Table);
	delete []candidates;
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void network_t::GetInputValue(const char * fileName)
{
	FILE * fp = fopen(fileName, "r");
	char buff[maxObjNum];
	int i, cnt;
	uint64_t f = 0;
	abc::Abc_Obj_t * pObj;
	while(fgets(buff, maxObjNum, fp) != nullptr){
		cnt = 2;
		assert((int)(strlen(buff)) == abc::Abc_NtkPiNum(pNtk) + 3);
		Abc_NtkForEachPi(pNtk, pObj, i) {
			if (buff[cnt] == '1') {
				SetBit(accurateValue[(pObj->Id << shiftNum) + (f >> 6)], f);
				SetBit(currentValue[(pObj->Id << shiftNum) + (f >> 6)], f);
				SetBit(tempValue[(pObj->Id << shiftNum) + (f >> 6)], f);
			}
			else {
				ClearBit(accurateValue[(pObj->Id << shiftNum) + (f >> 6)], f);
				ClearBit(currentValue[(pObj->Id << shiftNum) + (f >> 6)], f);
				ClearBit(tempValue[(pObj->Id << shiftNum) + (f >> 6)], f);
			}
			++cnt;
		}
		++f;
	}
	assert((int)f == simulationFrameNum);
	fclose(fp);

	for (int fb = 0; fb < frameBaseNum; ++fb) {
		accurateValue[(pOne->Id << shiftNum) + fb] = 0xffffffffffffffff;
		accurateValue[(pZero->Id << shiftNum) + fb] = 0;
		currentValue[(pOne->Id << shiftNum) + fb] = 0xffffffffffffffff;
		currentValue[(pZero->Id << shiftNum) + fb] = 0;
		tempValue[(pOne->Id << shiftNum) + fb] = 0xffffffffffffffff;
		tempValue[(pZero->Id << shiftNum) + fb] = 0;
	}
}

/**Function*************************************************************

  Synopsis    [Bit-based logic simulation]

  Description [
  1. uint64_t * p:
    (a) p is an array of values of each nodes in every frame base.
    (b) One frame base means 64 frames, for example, frame base 0 is from frame 0 to frame 63,
    frame base 1 is from frame 64 to frame 127.
    (c) p[(pObj->Id << shiftNum) + fb] denotes values of node pObj in the fb-th frame base.
    You can get the value of node pObj in the (fb * 64 + i)-th frame by:
    GetBit(p[(pObj->Id << shiftNum) + fb], i)
    You can set or reset the value of node pObj in the (fb * 64 + i)-th frame by:
    SetBit(p[(pObj->Id << shiftNum) + fb], i)
    ResetBit(p[(pObj->Id << shiftNum) + fb], i)
    (d) p could be network_t::accurateValue, network_t::currentValue, network_t::tempValue,
    where accurateValue is the values of original accurate circuit,
    currentValue is the values of intermediate approximate circuit,
    tempValue is used to backup currentValue in batch error estimation.
  2. Firstly, we get topological sequence vNodes. Secondly, the values of each nodes and each
    frame base are updated according to the gate types. Finally, the values are propagated to
    primary outputs.
  ]

***********************************************************************/
void network_t::GetAllValue(uint64_t * p)
{
	abc::Abc_Obj_t * pObj;
	int i;
	abc::Vec_Ptr_t * vNodes = GetTopoSequence(pNtk, 0);
	Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pObj, i) {
		switch ( gateType[pObj->Id] ) {
			case gate_t::BUF:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb];
			break;
			case gate_t::INV:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb];
			break;
			case gate_t::XOR:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] ^ p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb];
			break;
			case gate_t::XNOR:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] ^ p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]);
			break;
			case gate_t::AND2:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb];
			break;
			case gate_t::OR2:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb];
			break;
			case gate_t::NAND2:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]);
			break;
			case gate_t::NAND3:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb]);
			break;
			case gate_t::NAND4:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin3(pObj)->Id << shiftNum) + fb]);
			break;
			case gate_t::NOR2:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]);
			break;
			case gate_t::NOR3:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb]);
			break;
			case gate_t::NOR4:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin3(pObj)->Id << shiftNum) + fb]);
			break;
			case gate_t::AOI21:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~((p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]) | p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb]);
			break;
			case gate_t::AOI22:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~((p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]) | (p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin3(pObj)->Id << shiftNum) + fb]));
			break;
			case gate_t::OAI21:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~((p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]) & p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb]);
			break;
			case gate_t::OAI22:
				for (int fb = 0; fb < frameBaseNum; ++fb)
					p[(pObj->Id << shiftNum) + fb] = ~((p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]) & (p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin3(pObj)->Id << shiftNum) + fb]));
			break;
			default:
				assert(0);
		}
	}
	Abc_NtkForEachPo(pNtk, pObj, i) {
		for (int fb = 0; fb < frameBaseNum; ++fb)
			p[(pObj->Id << shiftNum) + fb] = p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb];
	}
	abc::Vec_PtrFree(vNodes);

	// check accurate value with c6288
	//for (int f = 0; f < simulationFrameNum; ++f) {
	//	int64_t f1, f2, res;
	//	f1 = f2 = res = 0;
	//	for ( i = Abc_NtkPiNum(pNtk) - 1; (i >= 0) && (((pObj) = Abc_NtkPi(pNtk, i)), 1); i-- ) {
	//		if (i < 16) {
	//			f1 <<= 1;
	//			f1 += ((accurateValue[(pObj->Id << shiftNum) + (f >> 6)] >> (f & (uint64_t)63)) & (uint64_t)1);
	//		}
	//		else {
	//			f2 <<= 1;
	//			f2 += ((accurateValue[(pObj->Id << shiftNum) + (f >> 6)] >> (f & (uint64_t)63)) & (uint64_t)1);
	//		}
	//	}

	//	res =((accurateValue[(abc::Abc_NtkPo(pNtk, 30)->Id << shiftNum) + (f >> 6)] >> (f & (uint64_t)63)) & (uint64_t)1);
	//	res <<= 1;
	//	res += ((accurateValue[(abc::Abc_NtkPo(pNtk, 31)->Id << shiftNum) + (f >> 6)] >> (f & (uint64_t)63)) & (uint64_t)1);
	//	for ( i = 29; (i >= 0) && (((pObj) = Abc_NtkPo(pNtk, i)), 1); i-- ) {
	//		res <<= 1;
	//		res += ((accurateValue[(pObj->Id << shiftNum) + (f >> 6)] >> (f & (uint64_t)63)) & (uint64_t)1);
	//	}
	//	std::cout << f1 << "\t" << f2 << "\t" << res << std::endl;
	//	assert(f1 * f2 == res);
	//}
}

void network_t::RenewCandidates(abc::Abc_Obj_t * pTS, abc::Abc_Obj_t * pSS)
{
	float er;
	GetAllValue(currentValue);
	er = GetErrorRate();
	addedER[(pTS->Id << shiftNum) + pSS->Id] = er;
	std::cout << Abc_ObjName(pTS) << "\t" << Abc_ObjName(pSS) << "\t" << er << std::endl;
	if (er < errorUpperBound) {
		candidates[candiNum].pTS = pTS;
		candidates[candiNum].pSS = pSS;
		candidates[candiNum].errorRate = er;
		candiNum++;
	}
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void network_t::SASIMI()
{
	int i, j;
	abc::Abc_Obj_t * pTS, * pSS, * pInv;
	abc::Vec_Ptr_t * vNodes;
	std::vector < std::vector < abc::Abc_Obj_t * > > recoverInfo;

	vNodes = GetTopoSequence(pNtk, 0);
    GetArrivalTime(pNtk);
	candiNum = 0;
	Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pTS, i) {
		// pTS replaced by 0/1
		TempReplace(pTS, pZero, recoverInfo);
		RenewCandidates(pTS, pZero);
		RecoverFromTemp(recoverInfo);

		TempReplace(pTS, pOne, recoverInfo);
		RenewCandidates(pTS, pOne);
		RecoverFromTemp(recoverInfo);

		Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pSS, j) {
			pInv = pInvs[pSS->Id];
            if (pInv == nullptr)
                continue;
			// pTS replaced by pSS
			if (pTS != pSS && Abc_NodeArrival(pTS) >= Abc_NodeArrival(pSS)) {
				TempReplace(pTS, pSS, recoverInfo);
				RenewCandidates(pTS, pSS);
				RecoverFromTemp(recoverInfo);
			}
			// pTS replaced by pSS and inverter
			if (pTS != pInv && pInvs[pTS->Id] != nullptr && Abc_NodeArrival(pTS) >= Abc_NodeArrival(pInv)) {
                // skip : a->b(inv)->c replaced by a->a's inv->c
				if (abc::Abc_ObjFanin0(pTS) == pSS && gateType[pTS->Id] == gate_t::INV)
					continue;
				TempReplace(pTS, pInv, recoverInfo);
				RenewCandidates(pTS, pInv);
				RecoverFromTemp(recoverInfo);
			}
		}
	}
	abc::Vec_PtrFree(vNodes);
    std::sort(candidates, candidates + candiNum);
}


/**Function*************************************************************

  Synopsis    [Advanced batch error estimation by deriving boolean difference.]

  Description [
  1. Get disjoint set (a special cut) of each node.
  2. Get boolean difference by logic simulation.
  3. Update increased error rate for each candidates.
  4. Save candidates satisfying the error rate constraint.
  ]

***********************************************************************/
void network_t::BatchErrorEstimationPro()
{
    abc::Vec_Ptr_t * vNodes = nullptr;
    abc::Abc_Obj_t * pObj = nullptr;
    abc::Abc_Obj_t * pInv = nullptr;
    abc::Abc_Obj_t * pNode = nullptr;
    abc::Abc_Obj_t * pTS = nullptr;
    abc::Abc_Obj_t * pSS = nullptr;
    abc::Abc_Obj_t * pFanout = nullptr;
    abc::Abc_Obj_t * pJointObj1 = nullptr;
    abc::Abc_Obj_t * pJointObj2 = nullptr;
    abc::Abc_Obj_t * pExpand = nullptr;
    std::list < abc::Abc_Obj_t * > disjointSet;
    std::list<abc::Abc_Obj_t *>::iterator iter1, iter2;
    uint64_t isCorrect = 0;
    uint64_t isWrong = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    int baseError = 0;
    bool isJoint = 0;

    // get topological sequence
    vNodes = GetTopoSequence(pNtk, 0);
    // renew fanout cone information
    memset(isPOInFOH, 0, sizeof(uint64_t) * maxObjNum);
    memset(isPOInFOM, 0, sizeof(uint64_t) * maxObjNum);
    memset(isPOInFOL, 0, sizeof(uint64_t) * maxObjNum);
    Abc_NtkForEachPo(pNtk, pObj, i) {
        if (i < 64)
            SetBit(isPOInFOL[pObj->Id], i);
        else if (i < 128)
            SetBit(isPOInFOM[pObj->Id], i - 64);
        else if (i < 192)
            SetBit(isPOInFOH[pObj->Id], i - 128);
        else
            assert(0);
    }
    Vec_PtrForEachEntryReverse(abc::Abc_Obj_t *, vNodes, pObj, i) {
        Abc_ObjForEachFanout(pObj, pFanout, j) {
            isPOInFOL[pObj->Id] |= isPOInFOL[pFanout->Id];
            isPOInFOM[pObj->Id] |= isPOInFOM[pFanout->Id];
            isPOInFOH[pObj->Id] |= isPOInFOH[pFanout->Id];
        }
    }
    // set stamp in topological sequence
    Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pObj, i)
        pObj->iTemp = i;
    Abc_NtkForEachPo(pNtk, pObj, i)
        pObj->iTemp = INT_MAX;
    // construct disjoint set
    Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pObj, i) {
        // clear list
        disjointSet.clear();
        abc::Vec_PtrClear(reconvPaths[pObj->Id]);
        abc::Vec_PtrClear(fringes[pObj->Id]);

        // set visited mark
        Vec_IntFill(&pNtk->vTravIds, Abc_NtkObjNumMax(pNtk)+500, 0);
        pNtk->nTravIds = 1;

        // add fanouts
        Abc_ObjForEachFanout(pObj, pFanout, j) {
            if ( !Abc_NodeIsTravIdCurrent(pFanout) && (Abc_ObjFanoutNum(pFanout) || Abc_ObjIsPo(pFanout)) ) {
                disjointSet.emplace_back(pFanout);
                //abc::Vec_PtrPush(reconvPaths[pObj->Id], pFanout);
                Abc_NodeSetTravIdCurrent(pFanout);
            }
        }

        // check whether joint
        isJoint = CheckJoint(disjointSet, iter1, iter2);

        // traverse
        while (isJoint) {
            pJointObj1 = *iter1;
            pJointObj2 = *iter2;
            // first expand nodes with smaller topological stamp
            if (pJointObj1->iTemp < pJointObj2->iTemp) {
                pExpand = pJointObj1;
                disjointSet.erase(iter1);
            }
            else {
                pExpand = pJointObj2;
                disjointSet.erase(iter2);
            }
            assert(pExpand != nullptr);
            Abc_ObjForEachFanout(pExpand, pFanout, j) {
                // if not visited and not dangling, add it
                if ( !Abc_NodeIsTravIdCurrent(pFanout) && (Abc_ObjFanoutNum(pFanout) || Abc_ObjIsPo(pFanout)) ) {
                    disjointSet.emplace_back(pFanout);
                    //abc::Vec_PtrPush(reconvPaths[pObj->Id], pFanout);
                    Abc_NodeSetTravIdCurrent(pFanout);
                }
            }
            // check whether joint
            isJoint = CheckJoint(disjointSet, iter1, iter2);
        }

        for (auto & pEle : disjointSet)
            abc::Vec_PtrPush(fringes[pObj->Id], pEle);
        Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pNode, j)
            if (Abc_NodeIsTravIdCurrent(pNode))
                abc::Vec_PtrPush(reconvPaths[pObj->Id], pNode);
        Abc_NtkForEachPo(pNtk, pNode, j)
            if (Abc_NodeIsTravIdCurrent(pNode))
                abc::Vec_PtrPush(reconvPaths[pObj->Id], pNode);

        //std::cout << Abc_ObjName(pObj) << "(" << abc::Vec_PtrSize(reconvPaths[pObj->Id]) << ")" << std::endl;
        //Vec_PtrForEachEntry(abc::Abc_Obj_t *, fringes[pObj->Id], pNode, j)
        //    std::cout << Abc_ObjName(pNode) << "\t";
        //std::cout << std::endl;
        //Vec_PtrForEachEntry(abc::Abc_Obj_t *, reconvPaths[pObj->Id], pNode, j)
        //    std::cout << Abc_ObjName(pNode) << "\t";
        //std::cout << std::endl;
    }

    // get current value and backup
	GetAllValue(currentValue);
    memcpy(tempValue, currentValue, sizeof(uint64_t) * maxObjNum * maxFrameNum >> 6);
    // get current error rate
    baseError = GetErrorRate();
    // get delay
    GetArrivalTime(pNtk);
    // init added error rate and boolean difference
	memset(addedER, 0, sizeof(int) * maxObjNum * maxObjNum);
    memset(CPM, 0, sizeof(uint64_t) * maxObjNum * maxPoNum);
    Abc_NtkForEachPo(pNtk, pTS, i)
		CPM[(pTS->Id << 8) + i] = 0xffffffffffffffff;

    // renew boolean difference
    for (int fb = 0; fb < frameBaseNum; ++fb) {
        // check PO correctness
		isCorrect = 0xffffffffffffffff;
		Abc_NtkForEachPo(pNtk, pTS, i) {
			isPoICorrect[i] = ~(currentValue[(pTS->Id << shiftNum) + fb] ^ accurateValue[(pTS->Id << shiftNum) + fb]);
			isCorrect &= isPoICorrect[i];
            //std::cout << GetBit(isPoICorrect[i], 3) << std::endl;
		}
		isWrong = ~isCorrect;
        //std::cout << GetBit(isWrong, 3) << std::endl;
		if (fb == frameBaseNum - 1) {
			isCorrect <<= (uint64_t)truncShift;
			isCorrect >>= (uint64_t)truncShift;
			isWrong <<= (uint64_t)truncShift;
			isWrong >>= (uint64_t)truncShift;
		}
        // get boolean difference
        Vec_PtrForEachEntryReverse(abc::Abc_Obj_t *, vNodes, pTS, i) {
            // clear
			PPM[pTS->Id] = 0;
			NPM[pTS->Id] = 0xffffffffffffffff;
			for (j = 0; j < Abc_NtkPoNum(pNtk); ++j)
				CPM[(pTS->Id << 8) + j] = 0;
            // flip
            currentValue[(pTS->Id << shiftNum) + fb] = ~currentValue[(pTS->Id << shiftNum) + fb];
            // simulate
            Vec_PtrForEachEntry(abc::Abc_Obj_t *, reconvPaths[pTS->Id], pObj , j) {
                if (Abc_ObjIsPo(pObj))
                    currentValue[(pObj->Id << shiftNum) + fb] = currentValue[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb];
                else
                    SimulateGate(currentValue, gateType[pObj->Id], pObj, shiftNum, fb);
            }
            // get change propagation matrix
            Vec_PtrForEachEntry(abc::Abc_Obj_t *, fringes[pTS->Id], pObj , j) {
                for ( k = 0; k < Abc_NtkPoNum(pNtk); ++k )
					CPM[(pTS->Id << 8) + k] |=
                        ((currentValue[(pObj->Id << shiftNum) + fb] ^ tempValue[(pObj->Id << shiftNum) + fb]) & CPM[(pObj->Id << 8) + k]);
            }
            for ( k = 0; k < Abc_NtkPoNum(pNtk); ++k ) {
                PPM[pTS->Id] |= CPM[(pTS->Id << 8) + k];
                NPM[pTS->Id] &= (isPoICorrect[k] ^ CPM[(pTS->Id << 8) + k]);
                //std::cout << Abc_ObjName(pTS) << "\t" << k << "\t" << isPoICorrect[k] << "\t" << CPM[(pTS->Id << 8) + k] << "\t" << PPM[pTS->Id] << "\t" << NPM[pTS->Id] << std::endl;
            }
            // recover
            Vec_PtrForEachEntry(abc::Abc_Obj_t *, reconvPaths[pTS->Id], pObj , j)
                currentValue[(pObj->Id << shiftNum) + fb] = tempValue[(pObj->Id << shiftNum) + fb];
            currentValue[(pTS->Id << shiftNum) + fb] = ~currentValue[(pTS->Id << shiftNum) + fb];

            //std::cout << Abc_ObjName(pTS) << ":\t";
            //for (j = 0; j < Abc_NtkPoNum(pNtk); ++j)
            //    std::cout << CPM[(pTS->Id << 8) + j] << "\t";
            //std::cout << std::endl;
        }

        // renew added error rate
        Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pTS, i) {
            uint64_t fc = isCorrect & PPM[pTS->Id];
            uint64_t fw = isWrong & NPM[pTS->Id];
            //std::cout << Abc_ObjName(pTS) << ":\t" << isCorrect << "\t" << isWrong << "\t" << fc << "\t" << fw << std::endl;
            // pTS replaced by 0/1
            uint64_t isDiff = currentValue[(pTS->Id << shiftNum) + fb];
			uint64_t flag = fc & isDiff;
            if (flag)
				addedER[(pTS->Id << shiftNum) + pZero->Id] += (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);
            flag = fw & isDiff;
            if (flag)
                addedER[(pTS->Id << shiftNum) + pZero->Id] -= (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);

            isDiff = ~currentValue[(pTS->Id << shiftNum) + fb];
            flag = fc & isDiff;
            if (flag)
				addedER[(pTS->Id << shiftNum) + pOne->Id] += (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);
            flag = fw & isDiff;
            if (flag)
                addedER[(pTS->Id << shiftNum) + pOne->Id] -= (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);

            Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pSS, j) {
                // if pSS is an added inverter, it will be included by its preditor
                pInv = pInvs[pSS->Id];
                if (pInv == nullptr)
                    continue;
                isDiff = currentValue[(pTS->Id << shiftNum) + fb] ^ currentValue[(pSS->Id << shiftNum) + fb];
                // pTS replaced by pSS
                if (pTS != pSS && Abc_NodeArrival(pTS) >= Abc_NodeArrival(pSS)) {
                    flag = fc & isDiff;
					if (flag)
						addedER[(pTS->Id << shiftNum) + pSS->Id] += (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);
                    flag = fw & isDiff;
                    if (flag)
                        addedER[(pTS->Id << shiftNum) + pSS->Id] -= (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);
                }
                // pTS replaced by pSS and inverter
                if (pTS != pInv && pInvs[pTS->Id] != nullptr && Abc_NodeArrival(pTS) >= Abc_NodeArrival(pInv)) {
                    // skip : a->b(inv)->c replaced by a->a's inv->c
                    if (abc::Abc_ObjFanin0(pTS) == pSS && gateType[pTS->Id] == gate_t::INV)
                        continue;
                    flag = fc & (~isDiff);
                    if (flag)
						addedER[(pTS->Id << shiftNum) + pInv->Id] += (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);
                    flag = fw & (~isDiff);
                    if (flag)
                        addedER[(pTS->Id << shiftNum) + pInv->Id] -= (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);
                }
            }

            Abc_NtkForEachPi(pNtk, pSS, j) {
                // if pSS is an added inverter, it will be included by its preditor
                pInv = pInvs[pSS->Id];
                if (pInv == nullptr)
                    continue;
                isDiff = currentValue[(pTS->Id << shiftNum) + fb] ^ currentValue[(pSS->Id << shiftNum) + fb];
                // pTS replaced by pSS
                if (pTS != pSS && Abc_NodeArrival(pTS) >= Abc_NodeArrival(pSS)) {
                    flag = fc & isDiff;
					if (flag)
						addedER[(pTS->Id << shiftNum) + pSS->Id] += (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);
                    flag = fw & isDiff;
                    if (flag)
                        addedER[(pTS->Id << shiftNum) + pSS->Id] -= (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);
                }
                // pTS replaced by pSS and inverter
                if (pTS != pInv && pInvs[pTS->Id] != nullptr && Abc_NodeArrival(pTS) >= Abc_NodeArrival(pInv)) {
                    // skip : a->b(inv)->c replaced by a->a's inv->c
                    if (abc::Abc_ObjFanin0(pTS) == pSS && gateType[pTS->Id] == gate_t::INV)
                        continue;
                    flag = fc & (~isDiff);
                    if (flag)
						addedER[(pTS->Id << shiftNum) + pInv->Id] += (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);
                    flag = fw & (~isDiff);
                    if (flag)
                        addedER[(pTS->Id << shiftNum) + pInv->Id] -= (count1Table[(flag >> 48) & 0xffff] + count1Table[(flag >> 32) & 0xffff] + count1Table[(flag >> 16) & 0xffff] + count1Table[flag & 0xffff]);
                }
            }

        }

    }

    // renew candidates
    candiNum = 0;
    Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pTS, i) {
        // pTS replaced by 0/1
        int er = baseError + addedER[(pTS->Id << shiftNum) + pZero->Id];
        if (er < errorUpperBound) {
            candidates[candiNum].pTS = pTS;
            candidates[candiNum].pSS = pZero;
            candidates[candiNum].errorRate = er;
            candiNum++;
        }
	    //std::cout << Abc_ObjName(pTS) << "\t" << Abc_ObjName(pZero) << "\t" << er << std::endl;

        er = baseError + addedER[(pTS->Id << shiftNum) + pOne->Id];
        if (er < errorUpperBound) {
            candidates[candiNum].pTS = pTS;
            candidates[candiNum].pSS = pOne;
            candidates[candiNum].errorRate = er;
            candiNum++;
        }
	    //std::cout << Abc_ObjName(pTS) << "\t" << Abc_ObjName(pOne) << "\t" << er << std::endl;

        Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pSS, j) {
            pInv = pInvs[pSS->Id];
            if (pInv == nullptr)
                continue;
            // pTS replaced by pSS
            if (pTS != pSS && Abc_NodeArrival(pTS) >= Abc_NodeArrival(pSS)) {
                er = baseError + addedER[(pTS->Id << shiftNum) + pSS->Id];
                if (er < errorUpperBound) {
                    candidates[candiNum].pTS = pTS;
                    candidates[candiNum].pSS = pSS;
                    candidates[candiNum].errorRate = er;
                    candiNum++;
                }
	            //std::cout << Abc_ObjName(pTS) << "\t" << Abc_ObjName(pSS) << "\t" << er << std::endl;
            }
            // pTS replaced by pSS and inverter
            if (pTS != pInv && pInvs[pTS->Id] != nullptr && Abc_NodeArrival(pTS) >= Abc_NodeArrival(pInv)) {
                // skip : a->b(inv)->c replaced by a->a's inv->c
                if (abc::Abc_ObjFanin0(pTS) == pSS && gateType[pTS->Id] == gate_t::INV)
                    continue;
                er = baseError + addedER[(pTS->Id << shiftNum) + pInv->Id];
                if (er < errorUpperBound) {
                    candidates[candiNum].pTS = pTS;
                    candidates[candiNum].pSS = pInv;
                    candidates[candiNum].errorRate = er;
                    candiNum++;
                }
	            //std::cout << Abc_ObjName(pTS) << "\t" << Abc_ObjName(pInv) << "\t" << er << std::endl;
            }
        }
        Abc_NtkForEachPi(pNtk, pSS, j) {
            pInv = pInvs[pSS->Id];
            if (pInv == nullptr)
                continue;
            // pTS replaced by pSS
            if (pTS != pSS && Abc_NodeArrival(pTS) >= Abc_NodeArrival(pSS)) {
                er = baseError + addedER[(pTS->Id << shiftNum) + pSS->Id];
                if (er < errorUpperBound) {
                    candidates[candiNum].pTS = pTS;
                    candidates[candiNum].pSS = pSS;
                    candidates[candiNum].errorRate = er;
                    candiNum++;
                }
	            //std::cout << Abc_ObjName(pTS) << "\t" << Abc_ObjName(pSS) << "\t" << er << std::endl;
            }
            // pTS replaced by pSS and inverter
            if (pTS != pInv && pInvs[pTS->Id] != nullptr && Abc_NodeArrival(pTS) >= Abc_NodeArrival(pInv)) {
                // skip : a->b(inv)->c replaced by a->a's inv->c
                if (abc::Abc_ObjFanin0(pTS) == pSS && gateType[pTS->Id] == gate_t::INV)
                    continue;
                er = baseError + addedER[(pTS->Id << shiftNum) + pInv->Id];
                if (er < errorUpperBound) {
                    candidates[candiNum].pTS = pTS;
                    candidates[candiNum].pSS = pInv;
                    candidates[candiNum].errorRate = er;
                    candiNum++;
                }
	            //std::cout << Abc_ObjName(pTS) << "\t" << Abc_ObjName(pInv) << "\t" << er << std::endl;
            }
        }

    }
    std::sort(candidates, candidates + candiNum);

    // free memory
    abc::Vec_PtrFree(vNodes);
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
bool network_t::CheckJoint(std::list <abc::Abc_Obj_t *> & disjointSet, std::list<abc::Abc_Obj_t *>::iterator &iter1, std::list<abc::Abc_Obj_t *>::iterator &iter2)
{
    abc::Abc_Obj_t * pObj1 = nullptr;
    abc::Abc_Obj_t * pObj2 = nullptr;
    for (iter1 = disjointSet.begin(); iter1 != disjointSet.end(); ++iter1) {
        pObj1 = *iter1;
        for (iter2 = iter1; iter2 != disjointSet.end(); ++iter2) {
            pObj2 = *iter2;
            if (pObj1 != pObj2) {
                if  ((isPOInFOL[pObj1->Id] & isPOInFOL[pObj2->Id]) ||
                     (isPOInFOM[pObj1->Id] & isPOInFOM[pObj2->Id]) ||
                     (isPOInFOH[pObj1->Id] & isPOInFOH[pObj2->Id]) ) {
                    //std::cout << "caution:" << Abc_ObjName(pObj1) << "\t" << Abc_ObjName(pObj2) << "\t" << isPOInFOL[pObj1->Id] << "\t" << isPOInFOL[pObj2->Id] << std::endl;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description [Compare accurateValue and currentValue]

***********************************************************************/
int network_t::GetErrorRate()
{
	abc::Abc_Obj_t * pObj;
	uint64_t temp;
	int i, erCnt = 0;
	for (int fb = 0; fb < frameBaseNum; ++fb) {
		temp = 0;
		Abc_NtkForEachPo(pNtk, pObj, i)
			temp |= (accurateValue[(pObj->Id << shiftNum) + fb] ^ currentValue[(pObj->Id << shiftNum) + fb]);
		if (fb == frameBaseNum - 1) {
			temp <<= (uint64_t)truncShift;
			temp >>= (uint64_t)truncShift;
		}
		erCnt += (count1Table[(temp >> 48) & 0xffff] + count1Table[(temp >> 32) & 0xffff] + count1Table[(temp >> 16) & 0xffff] + count1Table[temp & 0xffff]);
	}
	return erCnt;
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void network_t::RenewConst( abc::Abc_Ntk_t * pNtk )
{
	abc::Abc_Obj_t * pObj;
	int i;
	pZero = pOne = nullptr;
	Abc_NtkForEachNode( pNtk, pObj, i ) {
		if ( GetGateType( pObj ) == gate_t::CONST0 )
			pZero = pObj;
		else if ( GetGateType( pObj ) == gate_t::CONST1 )
			pOne = pObj;
	}
	if ( pZero == nullptr )
		pZero = Abc_NtkCreateNodeConst0( pNtk );
	if ( pOne == nullptr )
		pOne = Abc_NtkCreateNodeConst1( pNtk );
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void network_t::AddInverters(abc::Abc_Ntk_t * pNtk)
{
	abc::Abc_Obj_t * pObj;
	int i;
	int nObjs = abc::Vec_PtrSize(pNtk->vObjs);
	for ( i = 0; i < maxObjNum; ++i )
		pInvs[i] = nullptr;
	for ( i = 0; (i < nObjs) && ((pObj = Abc_NtkObj(pNtk, i)), 1); i++ )
        if ( pObj != nullptr && (Abc_ObjIsNode(pObj) || Abc_ObjIsPi(pObj)) && !Abc_NodeIsConst(pObj) )
			pInvs[pObj->Id] = Abc_NtkCreateNodeInv( pNtk, pObj );
	//Abc_NtkForEachObj(pNtk, pObj, i) {
	//	std::cout << pObj->Id << "\t" << Abc_ObjName(pObj) << "\t" << std::endl;
	//	if (pInvs[pObj->Id] != nullptr)
	//		std::cout << Abc_ObjName(pInvs[pObj->Id]) << std::endl;
	//}
	//assert(0);
}

/**Function*************************************************************

  Synopsis    []

  Description [mode = 0, don't collect 0/1; mode = 1, collect 0/1]

***********************************************************************/
abc::Vec_Ptr_t * network_t::GetTopoSequence(abc::Abc_Ntk_t * pNtk, int mode)
{
	abc::Vec_Ptr_t * vNodes;
    abc::Abc_Obj_t * pObj;
    int i;
    // set the traversal ID
    Vec_IntFill(&pNtk->vTravIds, Abc_NtkObjNumMax(pNtk)+500, 0);
    pNtk->nTravIds = 1;
    // start the array of nodes
    vNodes = abc::Vec_PtrAlloc( 100 );
    if (mode) {
		abc::Vec_PtrPush( vNodes, pZero );
		abc::Vec_PtrPush( vNodes, pOne );
   	}
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        abc::Abc_NodeSetTravIdCurrent( pObj );
        NtkDfs( Abc_ObjFanin0Ntk(abc::Abc_ObjFanin0(pObj)), vNodes );
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void network_t::PrintInfo()
{
	int cnt[19];
	abc::Abc_Obj_t * pObj;
	int i;
	memset(cnt, 0, sizeof(int) * 19);
	Abc_NtkForEachNode(pNtk, pObj, i)
		++cnt[(int)GetGateType(pObj)];
	for (int i = 1; i < 19; ++i)
		std::cout << (gate_t)i << "\t" << cnt[i] << std::endl;
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void NtkDfs( abc::Abc_Obj_t * pNode, abc::Vec_Ptr_t * vNodes )
{
	abc::Abc_Obj_t * pFanin;
    int i;
    assert( !abc::Abc_ObjIsNet(pNode) );
    // if this node is already visited, skip
    if ( abc::Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    // mark the node as visited
    abc::Abc_NodeSetTravIdCurrent( pNode );
    // skip the CI
    if ( abc::Abc_ObjIsCi(pNode) || (abc::Abc_NtkIsStrash(pNode->pNtk) && abc::Abc_AigNodeIsConst(pNode)) )
        return;
    assert( abc::Abc_ObjIsNode( pNode ) || abc::Abc_ObjIsBox( pNode ) );
    // visit the transitive fanin of the node
    Abc_ObjForEachFanin( pNode, pFanin, i )
        NtkDfs( abc::Abc_ObjFanin0Ntk(pFanin), vNodes );
    // add the node after the fanins have been added
    if (vNodes != nullptr && !Abc_NodeIsConst(pNode))
    	abc::Vec_PtrPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void NtkDfsReverse( abc::Abc_Obj_t * pNode )
{
	abc::Abc_Obj_t * pFanout;
    int i;
    assert( !abc::Abc_ObjIsNet(pNode) );
    // if this node is already visited or dangling, skip
    if ( abc::Abc_NodeIsTravIdCurrent(pNode) || !Abc_ObjFanoutNum(pNode) )
        return;
    // mark the node as visited
    abc::Abc_NodeSetTravIdCurrent( pNode );
    // skip the CO
    if ( abc::Abc_ObjIsCo(pNode) || (abc::Abc_NtkIsStrash(pNode->pNtk) && abc::Abc_AigNodeIsConst(pNode)) )
        return;
    assert( abc::Abc_ObjIsNode( pNode ) || abc::Abc_ObjIsBox( pNode ) );
    // visit the transitive fanout of the node
    Abc_ObjForEachFanout( pNode, pFanout, i )
        NtkDfsReverse(pFanout);
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
float GetArea(abc::Abc_Ntk_t * pNtk)
{
    abc::Vec_Ptr_t * vNodes = abc::Abc_NtkDfs(pNtk, 0);
    float area = 0.0;
    abc::Abc_Obj_t * pObj;
    int i;
    Vec_PtrForEachEntry(abc::Abc_Obj_t *, vNodes, pObj, i) {
        area += abc::Mio_GateReadArea( (abc::Mio_Gate_t *)pObj->pData );
        // std::cout << Abc_ObjName(pObj) << "\t" << abc::Mio_GateReadArea( (abc::Mio_Gate_t *)pObj->pData ) << std::endl;
    }
    abc::Vec_PtrFree(vNodes);
    return area;
}

/**Function*************************************************************

  Synopsis    [Remove NULL objects in the network, make sure ids of objects are continuous.]

  Description []

***********************************************************************/
abc::Abc_Ntk_t * CleanUpNetwork( abc::Abc_Ntk_t * pNtk )
{
	abc::Abc_Ntk_t * pNtkTmp = abc::Abc_NtkDup(pNtk);
	abc::Abc_NtkDelete(pNtk);
	pNtk = abc::Abc_NtkDup(pNtkTmp);
	abc::Abc_NtkDelete(pNtkTmp);
	return pNtk;
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void TempReplace( abc::Abc_Obj_t * pNodeOld, abc::Abc_Obj_t * pNodeNew, std::vector < std::vector < abc::Abc_Obj_t * > > & recoverInfo )
{
	// clear recover information
	std::vector < std::vector < abc::Abc_Obj_t * > >().swap( recoverInfo );
	// record recover information
	abc::Abc_Obj_t * pFanout, * pFanin;
	int i, j;
	std::vector < abc::Abc_Obj_t * > item;
	Abc_ObjForEachFanout( pNodeOld, pFanout, i ) {
		std::vector < abc::Abc_Obj_t * >().swap( item );
		item.emplace_back( pFanout );
		Abc_ObjForEachFanin( pFanout, pFanin, j )
			item.emplace_back( pFanin );
		recoverInfo.emplace_back( item );
	}
	Abc_ObjTransferFanout( pNodeOld, pNodeNew );
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void RecoverFromTemp( std::vector < std::vector < abc::Abc_Obj_t * > > & recoverInfo )
{
	abc::Abc_Obj_t * pFanout;
	for ( auto & item : recoverInfo ) {
		pFanout = item[0];
		Abc_ObjRemoveFanins( pFanout );
		for ( uint32_t i = 1; i < item.size(); ++i )
			Abc_ObjAddFanin( pFanout, item[i] );
	}
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void ShowNtk( abc::Abc_Ntk_t * pNtk0, char * FileNameDot )
{
    FILE * pFile;
    abc::Abc_Ntk_t * pNtk;
    abc::Abc_Obj_t * pNode;
    abc::Vec_Ptr_t * vNodes;
    int nBarBufs;
    int i;

    assert( abc::Abc_NtkIsStrash(pNtk0) || abc::Abc_NtkIsLogic(pNtk0) );
    if ( abc::Abc_NtkIsStrash(pNtk0) && abc::Abc_NtkGetChoiceNum(pNtk0) )
    {
        printf( "Temporarily visualization of AIGs with choice nodes is disabled.\n" );
        return;
    }
    // check that the file can be opened
    if ( (pFile = fopen( FileNameDot, "w" )) == nullptr )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }
    fclose( pFile );

    // convert to logic SOP
    pNtk = abc::Abc_NtkDup( pNtk0 );
    if ( abc::Abc_NtkIsLogic(pNtk) && !abc::Abc_NtkHasMapping(pNtk) )
        abc::Abc_NtkToSop( pNtk, -1, ABC_INFINITY );

    // collect all nodes in the network
    vNodes = abc::Vec_PtrAlloc( 100 );
    Abc_NtkForEachObj( pNtk, pNode, i )
        abc::Vec_PtrPush( vNodes, pNode );
    // write the DOT file
    nBarBufs = pNtk->nBarBufs;
    pNtk->nBarBufs = 0;
    WriteDotNtk( pNtk, vNodes, nullptr, FileNameDot, 1, 0 );
    pNtk->nBarBufs = nBarBufs;
    abc::Vec_PtrFree( vNodes );

    abc::Abc_NtkDelete( pNtk );
}

/**Function*************************************************************

  Synopsis    [Writes the graph structure of network for DOT.]

  Description [Useful for graph visualization using tools such as GraphViz:
  http://www.graphviz.org/]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void WriteDotNtk( abc::Abc_Ntk_t * pNtk, abc::Vec_Ptr_t * vNodes, abc::Vec_Ptr_t * vNodesShow, char * pFileName, int fGateNames, int fUseReverse )
{
    FILE * pFile;
    abc::Abc_Obj_t * pNode, * pFanin;
    char * pSopString;
    int LevelMin, LevelMax, fHasCos, Level, i, k, fHasBdds, fCompl, Prev;
    int Limit = 500;

    assert( Abc_NtkIsStrash(pNtk) || Abc_NtkIsLogic(pNtk) );

    if ( vNodes->nSize < 1 )
    {
        printf( "The set has no nodes. DOT file is not written.\n" );
        return;
    }

    if ( vNodes->nSize > Limit )
    {
        printf( "The set has more than %d nodes. DOT file is not written.\n", Limit );
        return;
    }

    // start the stream
    if ( (pFile = fopen( pFileName, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", pFileName );
        return;
    }

    // transform logic functions from BDD to SOP
    if ( (fHasBdds = Abc_NtkIsBddLogic(pNtk)) )
    {
        if ( !Abc_NtkBddToSop(pNtk, -1, ABC_INFINITY) )
        {
            printf( "Io_WriteDotNtk(): Converting to SOPs has failed.\n" );
            return;
        }
    }

    // mark the nodes from the set
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        pNode->fMarkC = 1;
    if ( vNodesShow )
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodesShow, pNode, i )
            pNode->fMarkB = 1;

    // get the levels of nodes
    LevelMax = Abc_NtkLevel( pNtk );
    if ( fUseReverse )
    {
        LevelMin = Abc_NtkLevelReverse( pNtk );
        assert( LevelMax == LevelMin );
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
            if ( Abc_ObjIsNode(pNode) )
                pNode->Level = LevelMax - pNode->Level + 1;
    }

    // find the largest and the smallest levels
    LevelMin = 10000;
    LevelMax = -1;
    fHasCos  = 0;
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( Abc_ObjIsCo(pNode) )
        {
            fHasCos = 1;
            continue;
        }
        if ( LevelMin > (int)pNode->Level )
            LevelMin = pNode->Level;
        if ( LevelMax < (int)pNode->Level )
            LevelMax = pNode->Level;
    }

    // set the level of the CO nodes
    if ( fHasCos )
    {
        LevelMax++;
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        {
            if ( Abc_ObjIsCo(pNode) )
                pNode->Level = LevelMax;
        }
    }

    // write the DOT header
    fprintf( pFile, "# %s\n",  "Network structure generated by ABC" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "digraph network {\n" );
    fprintf( pFile, "size = \"7.5,10\";\n" );
//    fprintf( pFile, "size = \"10,8.5\";\n" );
//    fprintf( pFile, "size = \"14,11\";\n" );
//    fprintf( pFile, "page = \"8,11\";\n" );
//  fprintf( pFile, "ranksep = 0.5;\n" );
//  fprintf( pFile, "nodesep = 0.5;\n" );
    fprintf( pFile, "center = true;\n" );
//    fprintf( pFile, "orientation = landscape;\n" );
//  fprintf( pFile, "edge [fontsize = 10];\n" );
//  fprintf( pFile, "edge [dir = none];\n" );
    fprintf( pFile, "edge [dir = back];\n" );
    fprintf( pFile, "\n" );

    // labels on the left of the picture
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  node [shape = plaintext];\n" );
    fprintf( pFile, "  edge [style = invis];\n" );
    fprintf( pFile, "  LevelTitle1 [label=\"\"];\n" );
    fprintf( pFile, "  LevelTitle2 [label=\"\"];\n" );
    // generate node names with labels
    for ( Level = LevelMax; Level >= LevelMin; Level-- )
    {
        // the visible node name
        fprintf( pFile, "  Level%d", Level );
        fprintf( pFile, " [label = " );
        // label name
        fprintf( pFile, "\"" );
        fprintf( pFile, "\"" );
        fprintf( pFile, "];\n" );
    }

    // genetate the sequence of visible/invisible nodes to mark levels
    fprintf( pFile, "  LevelTitle1 ->  LevelTitle2 ->" );
    for ( Level = LevelMax; Level >= LevelMin; Level-- )
    {
        // the visible node name
        fprintf( pFile, "  Level%d",  Level );
        // the connector
        if ( Level != LevelMin )
            fprintf( pFile, " ->" );
        else
            fprintf( pFile, ";" );
    }
    fprintf( pFile, "\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate title box on top
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  rank = same;\n" );
    fprintf( pFile, "  LevelTitle1;\n" );
    fprintf( pFile, "  title1 [shape=plaintext,\n" );
    fprintf( pFile, "          fontsize=20,\n" );
    fprintf( pFile, "          fontname = \"Times-Roman\",\n" );
    fprintf( pFile, "          label=\"" );
    fprintf( pFile, "%s", "Network structure visualized by ABC" );
    fprintf( pFile, "\\n" );
    fprintf( pFile, "Benchmark \\\"%s\\\". ", pNtk->pName );
    fprintf( pFile, "Time was %s. ",  abc::Extra_TimeStamp() );
    fprintf( pFile, "\"\n" );
    fprintf( pFile, "         ];\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate statistics box
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  rank = same;\n" );
    fprintf( pFile, "  LevelTitle2;\n" );
    fprintf( pFile, "  title2 [shape=plaintext,\n" );
    fprintf( pFile, "          fontsize=18,\n" );
    fprintf( pFile, "          fontname = \"Times-Roman\",\n" );
    fprintf( pFile, "          label=\"" );
    if ( Abc_NtkObjNum(pNtk) == Vec_PtrSize(vNodes) )
        fprintf( pFile, "The network contains %d logic nodes and %d latches.", Abc_NtkNodeNum(pNtk), Abc_NtkLatchNum(pNtk) );
    else
        fprintf( pFile, "The set contains %d logic nodes and spans %d levels.", NtkCountLogicNodes(vNodes), LevelMax - LevelMin + 1 );
    fprintf( pFile, "\\n" );
    fprintf( pFile, "\"\n" );
    fprintf( pFile, "         ];\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate the POs
    if ( fHasCos )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  LevelMax );
        // generate the PO nodes
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        {
            if ( !Abc_ObjIsCo(pNode) )
                continue;
            fprintf( pFile, "  Node%d [label = \"%s%s\"",
                pNode->Id,
                (Abc_ObjIsBi(pNode)? Abc_ObjName(Abc_ObjFanout0(pNode)):Abc_ObjName(pNode)),
                (Abc_ObjIsBi(pNode)? "_in":"") );
            fprintf( pFile, ", shape = %s", (Abc_ObjIsBi(pNode)? "box":"invtriangle") );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, ", color = coral, fillcolor = coral" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate nodes of each rank
    for ( Level = LevelMax - fHasCos; Level >= LevelMin && Level > 0; Level-- )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  Level );
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        {
            if ( (int)pNode->Level != Level )
                continue;
            if ( Abc_ObjFaninNum(pNode) == 0 )
                continue;

/*
            int SuppSize;
            Vec_Ptr_t * vSupp;
            if ( (int)pNode->Level != Level )
                continue;
            if ( Abc_ObjFaninNum(pNode) == 0 )
                continue;
            vSupp = Abc_NtkNodeSupport( pNtk, &pNode, 1 );
            SuppSize = Vec_PtrSize( vSupp );
            Vec_PtrFree( vSupp );
*/

//            fprintf( pFile, "  Node%d [label = \"%d\"", pNode->Id, pNode->Id );
            if ( Abc_NtkIsStrash(pNtk) ) {}
                //pSopString = "";
            else if ( Abc_NtkHasMapping(pNtk) && fGateNames )
                pSopString = Mio_GateReadName((abc::Mio_Gate_t *)pNode->pData);
            else if ( Abc_NtkHasMapping(pNtk) )
                pSopString = NtkPrintSop(abc::Mio_GateReadSop((abc::Mio_Gate_t *)pNode->pData));
            else
                pSopString = NtkPrintSop((char *)pNode->pData);
            //fprintf( pFile, "  Node%d [label = \"%d\\n%s\"", pNode->Id, pNode->Id, pSopString );
            fprintf( pFile, "  Node%d [label = \"%s\\n%s\"", pNode->Id, Abc_ObjName(pNode), pSopString );
//            fprintf( pFile, "  Node%d [label = \"%d\\n%s\"", pNode->Id,
//                SuppSize,
//                pSopString );

            fprintf( pFile, ", shape = ellipse" );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate the PI nodes if any
    if ( LevelMin == 0 )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  LevelMin );
        // generate the PO nodes
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        {
            if ( !Abc_ObjIsCi(pNode) )
            {
                // check if the costant node is present
                if ( Abc_ObjFaninNum(pNode) == 0 && Abc_ObjFanoutNum(pNode) > 0 )
                {
                    fprintf( pFile, "  Node%d [label = \"Const%d %s\"", pNode->Id, Abc_NtkIsStrash(pNode->pNtk) || Abc_NodeIsConst1(pNode), Abc_ObjName(pNode) );
                    fprintf( pFile, ", shape = ellipse" );
                    if ( pNode->fMarkB )
                        fprintf( pFile, ", style = filled" );
                    fprintf( pFile, ", color = coral, fillcolor = coral" );
                    fprintf( pFile, "];\n" );
                }
                continue;
            }
            fprintf( pFile, "  Node%d [label = \"%s\"",
                pNode->Id,
                (Abc_ObjIsBo(pNode)? Abc_ObjName(Abc_ObjFanin0(pNode)):Abc_ObjName(pNode)) );
            fprintf( pFile, ", shape = %s", (Abc_ObjIsBo(pNode)? "box":"triangle") );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, ", color = coral, fillcolor = coral" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate invisible edges from the square down
    fprintf( pFile, "title1 -> title2 [style = invis];\n" );
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( (int)pNode->Level != LevelMax )
            continue;
        fprintf( pFile, "title2 -> Node%d [style = invis];\n", pNode->Id );
    }
    // generate invisible edges among the COs
    Prev = -1;
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( (int)pNode->Level != LevelMax )
            continue;
        if ( !Abc_ObjIsPo(pNode) )
            continue;
        if ( Prev >= 0 )
            fprintf( pFile, "Node%d -> Node%d [style = invis];\n", Prev, pNode->Id );
        Prev = pNode->Id;
    }

    // generate edges
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( Abc_ObjIsLatch(pNode) )
            continue;
        Abc_ObjForEachFanin( pNode, pFanin, k )
        {
            if ( Abc_ObjIsLatch(pFanin) )
                continue;
            fCompl = 0;
            if ( Abc_NtkIsStrash(pNtk) )
                fCompl = Abc_ObjFaninC(pNode, k);
            // generate the edge from this node to the next
            fprintf( pFile, "Node%d",  pNode->Id );
            fprintf( pFile, " -> " );
            fprintf( pFile, "Node%d",  pFanin->Id );
            fprintf( pFile, " [style = %s", fCompl? "dotted" : "solid" );
//            fprintf( pFile, ", label = \"%c\"", 'a' + k );
            fprintf( pFile, "]" );
            fprintf( pFile, ";\n" );
        }
    }

    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );
    fclose( pFile );

    // unmark the nodes from the set
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        pNode->fMarkC = 0;
    if ( vNodesShow )
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodesShow, pNode, i )
            pNode->fMarkB = 0;

    // convert the network back into BDDs if this is how it was
    if ( fHasBdds )
        Abc_NtkSopToBdd(pNtk);
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
char * NtkPrintSop( char * pSop )
{
    static char Buffer[1000];
    char * pGet, * pSet;
    pSet = Buffer;
    for ( pGet = pSop; *pGet; pGet++ )
    {
        if ( *pGet == '\n' )
        {
            *pSet++ = '\\';
            *pSet++ = 'n';
        }
        else
            *pSet++ = *pGet;
    }
    *(pSet-2) = 0;
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
int NtkCountLogicNodes( abc::Vec_Ptr_t * vNodes )
{
    abc::Abc_Obj_t * pObj;
    int i, Counter = 0;
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pObj, i )
    {
        if ( !Abc_ObjIsNode(pObj) )
            continue;
        if ( Abc_ObjFaninNum(pObj) == 0 && Abc_ObjFanoutNum(pObj) == 0 )
            continue;
        Counter ++;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void ExtractFanioCone(abc::Abc_Ntk_t * pNtk0, char * nodeName)
{
    abc::Abc_Ntk_t * pNtk = nullptr;
    abc::Abc_Obj_t * pObj = nullptr;
    char fileName[100];
    int i;

    pNtk = abc::Abc_NtkDup(pNtk0);
    Abc_NtkForEachNode(pNtk, pObj, i) {
        if (!strcmp(Abc_ObjName(pObj), nodeName))
            break;
    }
    assert(!strcmp(Abc_ObjName(pObj), nodeName));
    // set the traversal ID
    Vec_IntFill(&pNtk->vTravIds, Abc_NtkObjNumMax(pNtk)+500, 0);
    pNtk->nTravIds = 1;
    // find fanout cone
    NtkDfsReverse(pObj);
    // delete not visited nodes
    Abc_NtkForEachNode(pNtk, pObj, i) {
        if (!abc::Abc_NodeIsTravIdCurrent(pObj))
            abc::Abc_NtkDeleteObj(pObj);
        //else
        //    std::cout << Abc_ObjName(pObj) << std::endl;
    }
    // visualize
    sprintf(fileName, "debug.dot");
    ShowNtk(pNtk, fileName);
    // free memory
    abc::Abc_NtkDelete(pNtk);
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void WriteBlif( abc::Abc_Ntk_t * pNtk, char * fileName )
{
	abc::Abc_Ntk_t * pNtkTemp = Abc_NtkToNetlist( Abc_NtkDup( pNtk ) );
    Io_WriteBlif( pNtkTemp, fileName, 1, 1, 1 );
	Abc_NtkDelete( pNtkTemp );
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void TempReplaceByName(abc::Abc_Ntk_t * pNtk, const char * pNameOld, const char * pNameNew)
{
	abc::Abc_Obj_t * pTS, * pSS;
	int i, j;
	std::vector < std::vector < abc::Abc_Obj_t * > > recoverInfo;
	Abc_NtkForEachNode(pNtk, pTS, i) {
		if (strcmp(abc::Abc_ObjName(pTS), pNameOld) == 0) {
            // std::cout << Abc_ObjName(pTS) << std::endl;
			Abc_NtkForEachNode(pNtk, pSS, j) {
				if (strcmp(Abc_ObjName(pSS), pNameNew) == 0) {
                    // std::cout << Abc_ObjName(pSS) << std::endl;
					TempReplace(pTS, pSS, recoverInfo);
                    return;
                }
            }
			Abc_NtkForEachPi(pNtk, pSS, j) {
				if (strcmp(Abc_ObjName(pSS), pNameNew) == 0) {
                    // std::cout << Abc_ObjName(pSS) << std::endl;
					TempReplace(pTS, pSS, recoverInfo);
                    return;
                }
            }
        }
    }
    assert(0);
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
void ReplaceByName(abc::Abc_Ntk_t * pNtk, const char * pNameOld, const char * pNameNew)
{
	abc::Abc_Obj_t * pTS, * pSS;
	int i, j;
	Abc_NtkForEachNode(pNtk, pTS, i) {
		if (strcmp(abc::Abc_ObjName(pTS), pNameOld) == 0) {
			Abc_NtkForEachNode(pNtk, pSS, j) {
				if (strcmp(Abc_ObjName(pSS), pNameNew) == 0) {
					Abc_ObjReplace(pTS, pSS);
				}
			}
		}
	}
}

float synthesis(abc::Abc_Frame_t * pAbc, abc::Abc_Ntk_t * pNtk, float originalDelay)
{
    abc::Abc_FrameSetCurrentNetwork(pAbc, Abc_NtkDup(pNtk));

    float delay, area, areaOld, delayOld;
    char Command[1000];
    areaOld = GetArea(Abc_FrameReadNtk(pAbc));
    delayOld = abc::GetArrivalTime(Abc_FrameReadNtk(pAbc));
    while (1) {
        sprintf( Command, "strash; balance; rewrite -l; refactor -l; balance; map -D %f", originalDelay );
        assert( !abc::Cmd_CommandExecute(pAbc, Command) );
        delay = abc::GetArrivalTime( Abc_FrameReadNtk( pAbc ));
        area = GetArea( Abc_FrameReadNtk( pAbc ) );
        if ((delay >= originalDelay + 0.01) || area >= areaOld)
            break;
        areaOld = area;
        delayOld = delay;
        std::cout << "rewrite -l, refactor -l : " << "area = " << areaOld << " delay = " << delayOld << std::endl;

        sprintf( Command, "strash; balance; rewrite -l; rewrite -lz; balance; map -D %f", originalDelay );
        assert( !abc::Cmd_CommandExecute(pAbc, Command) );
        delay = abc::GetArrivalTime( Abc_FrameReadNtk( pAbc ));
        area = GetArea( Abc_FrameReadNtk( pAbc ) );
        if ((delay >= originalDelay + 0.01) || area >= areaOld)
            break;
        areaOld = area;
        delayOld = delay;
        std::cout << "rewrite -l, rewrite -lz : " << "area = " << areaOld << " delay = " << delayOld << std::endl;

        sprintf( Command, "strash; balance; refactor -lz; rewrite -lz; balance; map -D %f", originalDelay );
        assert( !abc::Cmd_CommandExecute(pAbc, Command) );
        delay = abc::GetArrivalTime( Abc_FrameReadNtk( pAbc ));
        area = GetArea( Abc_FrameReadNtk( pAbc ) );
        if ((delay >= originalDelay + 0.01) || area >= areaOld)
            break;
        areaOld = area;
        delayOld = delay;
        std::cout << "refactor -lz, rewrite -lz : " << "area = " << areaOld << " delay = " << delayOld << std::endl;
    }

    std::cout << "area = "<< areaOld << " delay = " << delayOld << std::endl;
    return areaOld;
}


}

namespace abc
{

float GetArrivalTime( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode, * pDriver;
    float tArrivalMax, arrivalTime;
    int i;
    // init time manager
    Abc_NtkTimePrepare( pNtk );
    // get all nodes in topological order
    vNodes = Abc_NtkDfs( pNtk, 1 );
    // get arrival time
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i ) {
        if (!Abc_NodeIsConst(pNode))
            Abc_NodeDelayTraceArrival( pNode, nullptr );
        // std::cout << Abc_ObjName(pNode) << "\t" << Abc_NodeArrival(pNode) << std::endl;
        // Abc_ObjForEachFanin(pNode, pFanin, j)
        //     std::cout << Abc_ObjName(pFanin) << "\t" << Abc_NodeArrival(pFanin) << std::endl;
        // std::cout << "-------------------------------" << std::endl;
    }
    Vec_PtrFree( vNodes );
    // get the latest arrival times
    tArrivalMax = -ABC_INFINITY;
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pDriver = Abc_ObjFanin0(pNode);
        arrivalTime = Abc_NodeArrival(pDriver);
        if ( tArrivalMax < arrivalTime )
            tArrivalMax = arrivalTime;
    }
    return tArrivalMax;
}

}
