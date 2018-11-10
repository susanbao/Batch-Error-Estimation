/*CppFile****************************************************************

  FileName    [node.cpp]

  Synopsis    []

  Author      [Chang Meng]

  Date        [09/08/2018 15:00:39]

***********************************************************************/

#include "node.h"

namespace user
{

/**Function*************************************************************

  Synopsis    [Get type of gate]

  Description [Network must be mapped and pObj is a node]

***********************************************************************/
gate_t GetGateType( abc::Abc_Obj_t * pObj )
{
	char * pSop  = abc::Mio_GateReadSop( (abc::Mio_Gate_t *)pObj->pData );
	if ( SopIsConst0(pSop) ) 
		return gate_t::CONST0;
	else if ( SopIsConst1(pSop) ) 
		return gate_t::CONST1;
	else if ( SopIsBuf(pSop) ) 
		return gate_t::BUF;
	else if ( SopIsInvGate(pSop) ) 
		return gate_t::INV;
	else if ( SopIsXorGate(pSop) ) 
		return gate_t::XOR;
	else if ( SopIsXnorGate(pSop) ) 
		return gate_t::XNOR;
	else if ( SopIsAndGate(pSop) ) { 
		assert( abc::Abc_SopGetVarNum( pSop ) == 2 );
		return gate_t::AND2;
	}
	else if ( SopIsOrGate(pSop) ) {
		assert( abc::Abc_SopGetVarNum( pSop ) == 2 );
		return gate_t::OR2;
	}
	else if ( SopIsNandGate(pSop) ) {
		assert( abc::Abc_SopGetVarNum( pSop ) <= 4 );
		return (gate_t)( (int)gate_t::NAND2 + abc::Abc_SopGetVarNum( pSop ) - 2 );
	}
	else if ( SopIsNorGate(pSop) ) {
		assert( abc::Abc_SopGetVarNum( pSop ) <= 4 );
		return (gate_t)( (int)gate_t::NOR2 + abc::Abc_SopGetVarNum( pSop ) - 2 );
	}
	else if ( SopIsAOI21Gate(pSop) ) 
		return gate_t::AOI21;
	else if ( SopIsAOI22Gate(pSop) ) 
		return gate_t::AOI22;
	else if ( SopIsOAI21Gate(pSop) ) 
		return gate_t::OAI21;
	else if ( SopIsOAI22Gate(pSop) ) 
		return gate_t::OAI22;		
	else {
		std::cout << abc::Abc_ObjName( pObj ) << std::endl;
		std::cout << pSop << std::endl;
		assert( 0 );
	}
}

/**Function*************************************************************

  Synopsis    [Checks if the cover is constant 0.]

  Description []
               
***********************************************************************/
bool SopIsConst0( char * pSop ) 
{
	return abc::Abc_SopIsConst0(pSop);
}

/**Function*************************************************************

  Synopsis    [Checks if the cover is constant 1.]

  Description []
               
***********************************************************************/
bool SopIsConst1( char * pSop ) 
{
	return abc::Abc_SopIsConst1(pSop);
}

/**Function*************************************************************

  Synopsis    [Checks if the gate is BUF.]

  Description []
               
***********************************************************************/
bool SopIsBuf( char * pSop )
{
	return abc::Abc_SopIsBuf(pSop);
}


/**Function*************************************************************

  Synopsis    [Checks if the gate is NOT.]

  Description []
               
***********************************************************************/
bool SopIsInvGate( char * pSop )
{
	return abc::Abc_SopIsInv(pSop);
}

/**Function*************************************************************

  Synopsis    [Checks if the gate is AND.]

  Description []
               
***********************************************************************/
bool SopIsAndGate( char * pSop )
{
	if ( !abc::Abc_SopIsComplement(pSop) ) {	//111 1
		char * pCur;
		if ( abc::Abc_SopGetCubeNum(pSop) != 1 )
			return 0;
		for ( pCur = pSop; *pCur != ' '; pCur++ )
			if ( *pCur != '1' )
			    return 0;
	}
	else {	//0-- 0\n-0- 0\n--0 0\n
		char * pCube, * pCur;
		int nVars, nLits;
		nVars = abc::Abc_SopGetVarNum( pSop );
		if ( nVars != abc::Abc_SopGetCubeNum(pSop) )
		    return 0;
		Abc_SopForEachCube( pSop, nVars, pCube )
		{
		    nLits = 0;
		    for ( pCur = pCube; *pCur != ' '; pCur++ ) {
				if ( *pCur ==  '0' ) 
					nLits ++;
				else if ( *pCur == '-' )
					continue;
				else
					return 0;
			}
		    if ( nLits != 1 )
		        return 0;
		}
	}
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks if the gate is OR.]

  Description []
               
***********************************************************************/
bool SopIsOrGate( char * pSop )
{
    if ( abc::Abc_SopIsComplement(pSop) ) {	//000 0
		char * pCur;
		if ( abc::Abc_SopGetCubeNum(pSop) != 1 )
			return 0;
		for ( pCur = pSop; *pCur != ' '; pCur++ )
			if ( *pCur != '0' )
			    return 0;
	}
	else {	//1-- 1\n-1- 1\n--1 1\n
		char * pCube, * pCur;
		int nVars, nLits;
		nVars = abc::Abc_SopGetVarNum( pSop );
		if ( nVars != abc::Abc_SopGetCubeNum(pSop) )
		    return 0;
		Abc_SopForEachCube( pSop, nVars, pCube )
		{
		    nLits = 0;
		    for ( pCur = pCube; *pCur != ' '; pCur++ ) {
				if ( *pCur ==  '1' ) 
					nLits ++;
				else if ( *pCur == '-' )
					continue;
				else
					return 0;
			}
		    if ( nLits != 1 )
		        return 0;
		}
	}
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks if the gate is NAND.]

  Description []
               
***********************************************************************/
bool SopIsNandGate( char * pSop )
{
    if ( abc::Abc_SopIsComplement(pSop) ) {	//111 0
		char * pCur;
		if ( abc::Abc_SopGetCubeNum(pSop) != 1 )
			return 0;
		for ( pCur = pSop; *pCur != ' '; pCur++ )
			if ( *pCur != '1' )
			    return 0;
	}
	else {	//0-- 1\n-0- 1\n--0 1\n
		char * pCube, * pCur;
		int nVars, nLits;
		nVars = abc::Abc_SopGetVarNum( pSop );
		if ( nVars != abc::Abc_SopGetCubeNum(pSop) )
		    return 0;
		Abc_SopForEachCube( pSop, nVars, pCube )
		{
		    nLits = 0;
		    for ( pCur = pCube; *pCur != ' '; pCur++ ) {
				if ( *pCur ==  '0' ) 
					nLits ++;
				else if ( *pCur == '-' )
					continue;
				else
					return 0;
			}
		    if ( nLits != 1 )
		        return 0;
		}
	}
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks if the gate is NOR.]

  Description []
               
***********************************************************************/
bool SopIsNorGate( char * pSop )
{
    if ( !abc::Abc_SopIsComplement(pSop) ) {	//000 1
		char * pCur;
		if ( abc::Abc_SopGetCubeNum(pSop) != 1 )
			return 0;
		for ( pCur = pSop; *pCur != ' '; pCur++ )
			if ( *pCur != '0' )
			    return 0;
	}
	else {	//1-- 0\n-1- 0\n--1 0\n
		char * pCube, * pCur;
		int nVars, nLits;
		nVars = abc::Abc_SopGetVarNum( pSop );
		if ( nVars != abc::Abc_SopGetCubeNum(pSop) )
		    return 0;
		Abc_SopForEachCube( pSop, nVars, pCube )
		{
		    nLits = 0;
		    for ( pCur = pCube; *pCur != ' '; pCur++ ) {
				if ( *pCur ==  '1' ) 
					nLits ++;
				else if ( *pCur == '-' )
					continue;
				else
					return 0;
			}
		    if ( nLits != 1 )
		        return 0;
		}
	}
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks if the gate is XOR.]

  Description []
               
***********************************************************************/
bool SopIsXorGate( char * pSop )
{
	if ( !abc::Abc_SopIsComplement(pSop) ) {	//01 1\n10 1\n
		char * pCube, * pCur;
		int nVars, nLits;
		nVars = abc::Abc_SopGetVarNum( pSop );
		if ( nVars != 2 )
		    return 0;
		Abc_SopForEachCube( pSop, nVars, pCube )
		{
		    nLits = 0;
		    for ( pCur = pCube; *pCur != ' '; pCur++ ) {
				if ( *pCur ==  '1' ) 
					nLits ++;
				else if ( *pCur == '-' )
					return 0;
			}
		    if ( nLits != 1 )
		        return 0;
		}
	}
	else
		return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks if the gate is XNOR.]

  Description []
               
***********************************************************************/
bool SopIsXnorGate( char * pSop )
{
	if ( strcmp( pSop, "00 1\n11 1\n" ) == 0 )
		return 1;
	else if ( strcmp( pSop, "11 1\n00 1\n" ) == 0 )
		return 1;
	else 
		return 0;
}

/**Function*************************************************************

  Synopsis    [Checks if the gate is AOI21.]

  Description []
  
***********************************************************************/
bool SopIsAOI21Gate( char * pSop )
{
	if ( strcmp( pSop, "-00 1\n0-0 1\n" ) == 0 )
		return 1;
	else 
		return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
bool SopIsAOI22Gate( char * pSop )
{
	if ( strcmp( pSop, "--11 0\n11-- 0\n" ) == 0 )
		return 1;
	else 
		return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
bool SopIsOAI21Gate( char * pSop )
{
	if ( strcmp( pSop, "--0 1\n00- 1\n" ) == 0 )
		return 1;
	else 
		return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
bool SopIsOAI22Gate( char * pSop )
{
	if ( strcmp( pSop, "--00 1\n00-- 1\n" ) == 0 )
		return 1;
	else 
		return 0;
}

}
