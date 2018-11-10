/*HeaderFile****************************************************************

  FileName    [network.h]

  Synopsis    []

  Author      [Chang Meng]

  Date        [09/08/2018 15:21:00]

***********************************************************************/
#pragma once

#include "node.h"

namespace user
{

/**Class*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
class candidate_t
{
public:
	abc::Abc_Obj_t * pTS;
	abc::Abc_Obj_t * pSS;
	int errorRate;
	uint32_t childId;
 	candidate_t( abc::Abc_Obj_t * pts = nullptr, abc::Abc_Obj_t * pss = nullptr, int er = 0 ) { pTS = pts; pSS = pss; errorRate = er; }
 	bool operator < ( const candidate_t & c ) { return errorRate < c.errorRate; }
};

/**Class*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
class network_t
{
public:
	const int maxObjNum = 4096;
	const int shiftNum = 12;
	const int maxPoNum = 256;
	const int maxFrameNum = 131072;
	const int count1TableLength = 65536;
	static uint64_t * accurateValue;
	static uint64_t * currentValue;
	static uint64_t * tempValue;
	static abc::Abc_Obj_t ** pInvs;
    static float originalDelay;
	static float originalArea;
	static float invDelay;
	static int errorUpperBound;
	static int simulationFrameNum;
    static int frameBaseNum;
    static int truncShift;
	int candiNum;
	abc::Abc_Ntk_t * pNtk;
	gate_t * gateType;
    abc::Vec_Ptr_t ** reconvPaths;
    abc::Vec_Ptr_t ** fringes;
    abc::Vec_Int_t ** POIds;
	uint64_t * CPM;
	uint64_t * PPM;
	uint64_t * NPM;
	uint64_t * isPoICorrect;
    uint64_t * isPOInFOH;
    uint64_t * isPOInFOM;
    uint64_t * isPOInFOL;
	int * addedER;
	int8_t * count1Table;
	candidate_t * candidates;
	abc::Abc_Obj_t * pZero, * pOne;

	explicit network_t(abc::Abc_Ntk_t * p_ntk, int simulation_frame_num, const char * file_name, float error_rate);
	~network_t();
	void GetInputValue(const char * fileName);
	void GetAllValue(uint64_t * p);
	void RenewCandidates(abc::Abc_Obj_t * pTS, abc::Abc_Obj_t * pSS);
	void SASIMI();
	void BatchErrorEstimationPro();
    bool CheckJoint(std::list <abc::Abc_Obj_t *> & disjointSet, std::list<abc::Abc_Obj_t *>::iterator &iter1, std::list<abc::Abc_Obj_t *>::iterator &iter2);
	int GetErrorRate();
	void RenewConst( abc::Abc_Ntk_t * pNtk );
	void AddInverters(abc::Abc_Ntk_t * pNtk);
	abc::Vec_Ptr_t * GetTopoSequence(abc::Abc_Ntk_t * pNtk, int mode);
	void PrintInfo();
};

void NtkDfs(abc::Abc_Obj_t * pNode, abc::Vec_Ptr_t * vNodes = nullptr);
void NtkDfsReverse(abc::Abc_Obj_t * pNode);
float GetArea(abc::Abc_Ntk_t * pNtk);
abc::Abc_Ntk_t * CleanUpNetwork( abc::Abc_Ntk_t * pNtk );
void TempReplace( abc::Abc_Obj_t * pNodeOld, abc::Abc_Obj_t * pNodeNew, std::vector < std::vector < abc::Abc_Obj_t * > > & recoverInfo );
void RecoverFromTemp( std::vector < std::vector < abc::Abc_Obj_t * > > & recoverInfo );
void ShowNtk( abc::Abc_Ntk_t * pNtk0, char * FileNameDot );
void ExtractFanioCone(abc::Abc_Ntk_t * pNtk0, char * nodeName);
void WriteDotNtk( abc::Abc_Ntk_t * pNtk, abc::Vec_Ptr_t * vNodes, abc::Vec_Ptr_t * vNodesShow, char * pFileName, int fGateNames, int fUseReverse );
char * NtkPrintSop( char * pSop );
int NtkCountLogicNodes( abc::Vec_Ptr_t * vNodes );
void WriteBlif( abc::Abc_Ntk_t * pNtk, char * fileName );
void TempReplaceByName(abc::Abc_Ntk_t * pNtk, const char * pNameOld, const char * pNameNew);
void ReplaceByName(abc::Abc_Ntk_t * pNtk, const char * pNameOld, const char * pNameNew);
float synthesis(abc::Abc_Frame_t * pAbc, abc::Abc_Ntk_t * pNtk, float originalDelay);

static inline void SetBit(uint64_t & x, uint64_t f) { x |= ((uint64_t)1 << (f & (uint64_t)63)); }

static inline void ClearBit(uint64_t & x, uint64_t f) { x &= ~((uint64_t)1 << (f & (uint64_t)63)); }

static inline bool GetBit(uint64_t x, uint64_t f) { return (bool)((x >> f) & (uint64_t)1); }

static inline uint64_t GetBD(gate_t gate_type, abc::Abc_Obj_t * pFanout, uint64_t * p, uint64_t fb, uint64_t shiftNum)
{
    switch ( gate_type ) {
		case gate_t::BUF:
		case gate_t::INV:
			return 0xffffffffffffffff;
		break;
		case gate_t::XOR:
			return (p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] ^ p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb]) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::XNOR:
			return (~(p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] ^ p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb])) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::AND2:
			return (p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb]) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::OR2:
			return (p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb]) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::NAND2:
			return (~(p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb])) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::NAND3:
			return (~(p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb] & p[(Abc_ObjFanin2(pFanout)->Id << shiftNum) + fb])) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::NAND4:
			return (~(p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb] & p[(Abc_ObjFanin2(pFanout)->Id << shiftNum) + fb] & p[(Abc_ObjFanin3(pFanout)->Id << shiftNum) + fb])) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::NOR2:
			return (~(p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb])) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::NOR3:
			return (~(p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb] | p[(Abc_ObjFanin2(pFanout)->Id << shiftNum) + fb])) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::NOR4:
			return (~(p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb] | p[(Abc_ObjFanin2(pFanout)->Id << shiftNum) + fb] | p[(Abc_ObjFanin3(pFanout)->Id << shiftNum) + fb])) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::AOI21:
			return (~((p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb]) | p[(Abc_ObjFanin2(pFanout)->Id << shiftNum) + fb])) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::AOI22:
			return (~((p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb]) | (p[(Abc_ObjFanin2(pFanout)->Id << shiftNum) + fb] & p[(Abc_ObjFanin3(pFanout)->Id << shiftNum) + fb]))) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::OAI21:
			return (~((p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb]) & p[(Abc_ObjFanin2(pFanout)->Id << shiftNum) + fb])) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		case gate_t::OAI22:
			return (~((p[(Abc_ObjFanin0(pFanout)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pFanout)->Id << shiftNum) + fb]) & (p[(Abc_ObjFanin2(pFanout)->Id << shiftNum) + fb] | p[(Abc_ObjFanin3(pFanout)->Id << shiftNum) + fb]))) ^ p[(pFanout->Id << shiftNum) + fb];
		break;
		default:
			assert(0);
	}
}

static inline void SimulateGate(uint64_t * p, gate_t & gate_type, abc::Abc_Obj_t * pObj, int shiftNum, int fb)
{
    switch ( gate_type ) {
        case gate_t::BUF:
			p[(pObj->Id << shiftNum) + fb] = p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb];
        break;
        case gate_t::INV:
			p[(pObj->Id << shiftNum) + fb] = ~p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb];
        break;
        case gate_t::XOR:
			p[(pObj->Id << shiftNum) + fb] = p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] ^ p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb];
        break;
        case gate_t::XNOR:
			p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] ^ p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]);
        break;
        case gate_t::AND2:
			p[(pObj->Id << shiftNum) + fb] = p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb];
        break;
        case gate_t::OR2:
			p[(pObj->Id << shiftNum) + fb] = p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb];
        break;
        case gate_t::NAND2:
			p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]);
        break;
        case gate_t::NAND3:
			p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb]);
        break;
        case gate_t::NAND4:
			p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin3(pObj)->Id << shiftNum) + fb]);
        break;
        case gate_t::NOR2:
			p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]);
        break;
        case gate_t::NOR3:
			p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb]);
        break;
        case gate_t::NOR4:
			p[(pObj->Id << shiftNum) + fb] = ~(p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin3(pObj)->Id << shiftNum) + fb]);
        break;
        case gate_t::AOI21:
			p[(pObj->Id << shiftNum) + fb] = ~((p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]) | p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb]);
        break;
        case gate_t::AOI22:
			p[(pObj->Id << shiftNum) + fb] = ~((p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]) | (p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb] & p[(Abc_ObjFanin3(pObj)->Id << shiftNum) + fb]));
        break;
        case gate_t::OAI21:
			p[(pObj->Id << shiftNum) + fb] = ~((p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]) & p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb]);
        break;
        case gate_t::OAI22:
			p[(pObj->Id << shiftNum) + fb] = ~((p[(Abc_ObjFanin0(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin1(pObj)->Id << shiftNum) + fb]) & (p[(Abc_ObjFanin2(pObj)->Id << shiftNum) + fb] | p[(Abc_ObjFanin3(pObj)->Id << shiftNum) + fb]));
        break;
        default:
            assert(0);
    }
}

}

namespace abc
{
    float GetArrivalTime( Abc_Ntk_t * pNtk );
}
