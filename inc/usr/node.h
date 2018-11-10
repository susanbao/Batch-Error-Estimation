/*HeaderFile****************************************************************

  FileName    [node.h]

  Synopsis    []

  Author      [Chang Meng]

  Date        [09/08/2018 15:06:16]

***********************************************************************/

#pragma once

#include <bits/stdc++.h>
#include "abc_api.h"

namespace user
{

/**Class*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
enum class gate_t
{
	UNKNOWN = 0, CONST0, CONST1,
	BUF,	INV,	XOR,
    XNOR,	AND2,	OR2,
    NAND2,	NAND3,	NAND4,
    NOR2,	NOR3,	NOR4,
	AOI21,	AOI22,	OAI21,	OAI22
};

gate_t GetGateType( abc::Abc_Obj_t * pObj );
bool SopIsConst0( char * pSop );
bool SopIsConst1( char * pSop );
bool SopIsBuf( char * pSop );
bool SopIsInvGate( char * pSop );
bool SopIsAndGate( char * pSop );
bool SopIsOrGate( char * pSop );
bool SopIsNandGate( char * pSop );
bool SopIsNorGate( char * pSop );
bool SopIsXorGate( char * pSop );
bool SopIsXnorGate( char * pSop );
bool SopIsAOI21Gate( char * pSop );
bool SopIsAOI22Gate( char * pSop );
bool SopIsOAI21Gate( char * pSop );
bool SopIsOAI22Gate( char * pSop );

static inline std::ostream & operator << (std::ostream & os, const gate_t & gateType)
{
	switch ( gateType ) {
		case gate_t::CONST0:
			std::cout << "const0";
		break;
		case gate_t::CONST1:
			std::cout << "const1";
		break;
    	case gate_t::BUF:
			std::cout << "buf";
    	break;
    	case gate_t::INV:
			std::cout << "inv";
    	break;
    	case gate_t::XOR:
			std::cout << "xor2";
    	break;
    	case gate_t::XNOR:
			std::cout << "xnor2";
    	break;
    	case gate_t::AND2:
			std::cout << "and2";
    	break;
    	case gate_t::OR2:
			std::cout << "or2";
    	break;
    	case gate_t::NAND2:
			std::cout << "nand2";
    	break;
    	case gate_t::NAND3:
			std::cout << "nand3";
    	break;
    	case gate_t::NAND4:
			std::cout << "nand4";
    	break;
    	case gate_t::NOR2:
			std::cout << "nor2";
    	break;
    	case gate_t::NOR3:
			std::cout << "nor3";
    	break;
    	case gate_t::NOR4:
			std::cout << "nor4";
    	break;
    	case gate_t::AOI21:
			std::cout << "aoi21";
    	break;
    	case gate_t::AOI22:
			std::cout << "aoi22";
    	break;
    	case gate_t::OAI21:
			std::cout << "oai21";
    	break;
    	case gate_t::OAI22:
			std::cout << "oai22";
    	break;
    	default:
    		assert( 0 );
    }
    return os;
}

}

namespace abc
{
static inline float Abc_NodeArrival( abc::Abc_Obj_t * pNode )  {  return ((abc::Abc_Time_t *)pNode->pNtk->pManTime->vArrs->pArray[pNode->Id])->Rise;  }
}
