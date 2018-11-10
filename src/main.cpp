/*CppFile****************************************************************

  fileName    [main.cpp]

  Synopsis    []

  Author      [Chang Meng]

  Date        [09/08/2018 11:24:09]

***********************************************************************/
#include "network.h"

/**Function*************************************************************

  Synopsis    []

  Description []

***********************************************************************/
int main( int argc, char * argv[] )
{
	// Parse input arguments
    if ( argc != 3 )
    {
        printf( "Usage:\n%s [circuit name] [error rate]\n", argv[0] );
        printf( "i.e., %s c432 0.01\n", argv[0] );
        return 1;
    }
	char * fileName = argv[1];
    float er = atof(argv[2]);
    char Command[1000];
    std::cout << "circuit : benchmark/" << fileName << ".blif" << std::endl;
    std::cout << "standard cell : lib/mcnc.genlib" << std::endl;
    std::cout << "error rate upper bound : " << er << std::endl;

	// init pseudo-random number generator
	srand((unsigned)time(NULL));
    // start the ABC framework
    abc::Abc_Start();
    abc::Abc_Frame_t * pAbc = abc::Abc_FrameGetGlobalFrame();
    // input the standard cell library
    sprintf( Command, "read_genlib -v %s", "lib/mcnc.genlib" );
    assert( !abc::Cmd_CommandExecute( pAbc, Command ) );
	// input the original circuit
	sprintf( Command, "read benchmark/%s.blif", fileName );
    assert( !abc::Cmd_CommandExecute( pAbc, Command ) );
	// build user's network from ABC network
	int frameNum = 100000;
	sprintf(Command, "in/%d/%s.in", frameNum, fileName);
	user::network_t ntk(Abc_FrameReadNtk(pAbc), frameNum, Command, er);
    // logic simulation
    ntk.GetAllValue(ntk.accurateValue);
    // batch error estimation
    ntk.BatchErrorEstimationPro();
    std::cout << "candidates of SASIMI:" << std::endl;
    std::cout << "TS\tSS\tER" << std::endl;
    for (int i = 0; i < ntk.candiNum; ++i) {
        std::cout << Abc_ObjName(ntk.candidates[i].pTS) << "\t" <<
            Abc_ObjName(ntk.candidates[i].pSS) << "\t" <<
            ntk.candidates[i].errorRate / (float)frameNum << std::endl;
    }

	// Stop ABC framework
	abc::Abc_Stop();
    // cleanup
    free(ntk.accurateValue);
    free(ntk.currentValue);
    free(ntk.tempValue);
    free(ntk.pInvs);

	return 0;
}
