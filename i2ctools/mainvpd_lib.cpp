#include "mainvpd_lib.h"

int readMain_I2C_eeprom (char * vpd_str , int board_id , int startAddress , int length);
#ifdef SP2x
	  uint8_t I2C_DC2DC_SWITCH = I2C_DC2DC_SWITCH_GROUP0;
#else
	  uint8_t I2C_DC2DC_SWITCH = I2C_DC2DC_SWITCH_GROUP1;
#endif

		const char *  FET_CODE_2_STR [] = {
				"72A", //0
				"72B", //1
				"72A3P", //2
				"72B3P", //3
				"72A50A", //4
				"72B50A", //5
				"N/A", //6
				"N/A", //7
				"78A50A", //8
				"78B50A", //9 integer (unsigned) [ 0=72A[4P40A] , 1=72B[4P40A],2=72A3P[50A],3=72B3P[50A],4=72A4P50A,5=72B4P50A,6=N/A,7=N/A,8=78A[4P50A],9=78B[4P50A],10=78A3P[50A],11=78B3P[50A] 100=N/A
				"78A3P", //10
				"78B3P", //11
				"N/A", //12
				"N/A", //13
				"N/A", //14
				"N/A" //15
		};

void resetI2CSwitches(){
	int err;
	i2c_write(I2C_DC2DC_SWITCH, PRIMARY_I2C_SWITCH_DEAULT , &err);
	usleep(10000);

	i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT , &err);
	usleep(10000);
}

inline int fix_max_cap(int val, int max){
	if (val < 0)
		return 0;
	else if (val > max)
		return max;
	else
		return val;
}

int setI2CSwitches(int board_id){

	int err = 0;
	int I2C_MY_MAIN_BOARD_PIN = (board_id == BOARD_0)?PRIMARY_I2C_SWITCH_BOARD0_MAIN_PIN : PRIMARY_I2C_SWITCH_BOARD1_MAIN_PIN;
	i2c_write(PRIMARY_I2C_SWITCH, I2C_MY_MAIN_BOARD_PIN | PRIMARY_I2C_SWITCH_DEAULT , &err);

	if (err==0){
		usleep(10000);
		i2c_write(I2C_DC2DC_SWITCH,  PRIMARY_I2C_SWITCH_DEAULT , &err);
	}
	else{
		fprintf(stderr,"failed calling i2c set 0x70 %x\n", I2C_MY_MAIN_BOARD_PIN | PRIMARY_I2C_SWITCH_DEAULT);
		return err;
	}

	if (err==0){
		usleep(10000);
		i2c_write(I2C_DC2DC_SWITCH,  MAIN_BOARD_I2C_SWITCH_EEPROM_PIN , &err);
		usleep(10000);
	}
	else{
		fprintf(stderr,"failed calling i2c set 0x71 %x\n", PRIMARY_I2C_SWITCH_DEAULT);
		return err;
	}

	return err;
}

int set_fet(int board_id , int fet_type){
	int err=0;
	int rc = 0;
	int vpdrev = 0;
	const char * fetstr = NULL;

	if (fet_type < 0 || fet_type > 15){
		fprintf(stderr , "FET TYPE %d is illegal (0-15)\n" , fet_type);
		return FET_ERROR_ILLEGAL_VALUE;
	}
	pthread_mutex_lock(&i2c_mutex);

	err = setI2CSwitches(board_id);


	if (err){
		rc = FET_ERROR_BOARD_NA;
		goto get_out;
	}

	vpdrev =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , &err);

	if (err){
		rc = FET_EEPROM_DEV_ERROR;
		goto get_out;
	}

	if (vpdrev == 0xFF){
		i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , MAIN_VPD_REV , &err);
		usleep(10000);
		for (int a = 1 ; a < MAIN_BOARD_VPD_EEPROM_SIZE ; a++){
			i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, a , 0 , &err);
			usleep(10000);
		}
	}
	else if ( vpdrev < MAIN_VPD_REV) {
		i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , MAIN_VPD_REV , &err);
		usleep(10000);
	}

	if (err){
		rc = FET_ERROR_VPD_READ_ERROR;
		goto get_out;
	}

	i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, MAIN_BOARD_VPD_FET_FLAG_ADDR_START , 1 , &err);
	usleep(10000);
	if (err){
		rc = FET_ERROR_VPD_WRITE_ERROR;
		goto get_out;
	}

	i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, MAIN_BOARD_VPD_FET_CODE_ADDR_START , fet_type , &err);
	usleep(10000);
	if (err){
		rc = FET_ERROR_VPD_WRITE_ERROR;
		goto get_out;
	}

	fetstr = FET_CODE_2_STR[fet_type];
	int b ;
	for (b = 0 ; b < strlen(fetstr); b++)
	{
		i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, b+MAIN_BOARD_VPD_FET_STR_ADDR_START  , fetstr[b] , &err);
		usleep(10000);
		if(err != 0){
			rc = FET_ERROR_VPD_WRITE_ERROR;
			goto get_out;
		}
	}
	i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, b+MAIN_BOARD_VPD_FET_STR_ADDR_START  , '\0' , &err);

	get_out:
	resetI2CSwitches();
	pthread_mutex_unlock(&i2c_mutex);
	return rc;
}

int is_sp30() {

	int rc = 0;
	int r;
	char model [16];
	FILE* file = fopen("/model_name", "r");
	if (file < 0) {
		psyslog("file /model_name missing is it an SP miner at all??");
	    passert(0);
	}
	else{
		r = fscanf (file,  "%s",model);
		if (r > 0){
			rc = (strcmp("SP30",model) == 0);
		}
	}
	fclose (file);
	return rc;
}


int get_fet_from_ela(mainboard_vpd_info_t * vpd){
	int fet_code = FET_ERROR_UNKNOWN_ELA;

	int pnr = -1;
	int rev = -1;

	// fprintf(stdout,"%s %s %s\n",vpd->serial,vpd->pnr,vpd->revision);
	sscanf (vpd->pnr,"ELA-%d",&pnr);
	sscanf (vpd->revision,"%*c%d",&rev);

	if (pnr == -1 || rev == -1){
		fprintf(stderr , "Failed to read PNR or REV values\n");
		return fet_code;
	}

	// fprintf (stdout,"PNR:%d REV:%d\n",pnr,rev);

    if (pnr == 2013) // ELA-2013 -> FET72A
    {
        if (rev < 3)
        {
            /// dc2dcSetupPart1 = string.Format("{0} {1}",Constants.MAIN_DC2DC_SETUP_CMD_72A_1,wiring);//"SetupDC2DC_72A_1_of_2";
            /// FETType = "72A";
            fet_code = FET_T_72A;
        }
        else
        {
            /// dc2dcSetupPart1 = string.Format("{0} {1}", Constants.MAIN_DC2DC_SETUP_CMD_72A3PH_1, wiring);//"SetupDC2DC_72A-3PH_1_of_2";
            /// FETType = "F72A3PH";
            fet_code = FET_T_72A_3PHASE;
        }
    }
    else if (pnr == 2003)
    {
        if (rev < 2)
        {
            /// dc2dcSetupPart1 = string.Format("{0} {1}",Constants.MAIN_DC2DC_SETUP_CMD_72B_1,wiring);// "SetupDC2DC_72B_1_of_2";
            /// FETType = "72B";
            fet_code = FET_T_72B;
        }
        else
        {
            /// dc2dcSetupPart1 = string.Format("{0} {1}",Constants.MAIN_DC2DC_SETUP_CMD_72B3PH_1,wiring);// "SetupDC2DC_72B-3PH_1_of_2";
            /// FETType = "72B3PH";
            fet_code = FET_T_72B_3PHASE;
        }
    }
    else if (pnr == 2023) // ELA-2023 -> FET78B
    {
        /// ELA-2023 78B
        /// A00 - 3 phases
        /// FET_TYPE:11
        /// conf:SetupDC2DC_78B-3PH_1_of_2

        /// dc2dcSetupPart1 = string.Format("{0} {1}", Constants.MAIN_DC2DC_SETUP_CMD_78B3PH_1, wiring);
        /// FETType = "78B3PH";
        fet_code = FET_T_78B_3PHASE;
    }
    else if (pnr == 2113) // ELA-2113 -> FET72A
    {
        ///
        /// ELA-2113 72A
        /// No BOM yet.
        /// (guess: FET_TYPE:2)
        /// (guess conf: SetupDC2DC_72A-3PH_1_of_2)
        ///

        /// dc2dcSetupPart1 = string.Format("{0} {1}", Constants.MAIN_DC2DC_SETUP_CMD_72A3PH_1, wiring);//"SetupDC2DC_72A_1_of_2";
        /// FETType = "72A3PH";
        fet_code = FET_T_72A_3PHASE;
    }
    else if (pnr == 2103)
    {
        if (rev < 1)
        {
            /// ELA-2103 72B
            /// A00 - 4 phases
            /// FET_TYPE:5
            /// conf-script: SetupDC2DC_72B-50A_1_of_2

            /// dc2dcSetupPart1 = string.Format("{0} {1}", Constants.MAIN_DC2DC_SETUP_CMD_72B50A_1, wiring);// "SetupDC2DC_72B_1_of_2";
            /// FETType = "72B4P50";
            fet_code = FET_T_72B_I50;
        }
        else
        {
            /// ELA-2103 72B 3P
            /// A01+ - 3 phases
            /// FET_TYPE:3
            /// conf-script: SetupDC2DC_72B3PH_1_of_2

            /// dc2dcSetupPart1 = string.Format("{0} {1}", Constants.MAIN_DC2DC_SETUP_CMD_72B3PH_1, wiring);// "SetupDC2DC_72B3PH_1_of_2";
            /// FETType = "72B3PH";
            fet_code = FET_T_72B_3PHASE;
        }
    }
    else if (pnr == 2123)
    {
        /// ELA-2123 78B
        /// A00 - 3 phases
        /// FET_TYPE:11
        /// conf:SetupDC2DC_78B-3PH_1_of_2

        /// dc2dcSetupPart1 = string.Format("{0} {1}", Constants.MAIN_DC2DC_SETUP_CMD_78B3PH_1, wiring);
        /// FETType = "78B3PH";
        fet_code = FET_T_78B_3PHASE;
    }
    else if (pnr == 1003)
    {
        /// ELA-1003 72B
        /// A00 - 4 phases
        /// FET_TYPE:5
        /// conf:SetupDC2DC_72B50A

        /// dc2dcSetupPart1 = string.Format("{0} {1} SP20", Constants.MAIN_DC2DC_SETUP_CMD_72B50A_1, wiring);
        /// FETType = "72B4P50";
        fet_code = FET_T_72B_I50;
    }
    else if (pnr == 1013)
    {
        /// ELA-1013 72A
        /// A00 - 4 phases
        /// FET_TYPE:4
        /// conf:SetupDC2DC_72A50A

        /// dc2dcSetupPart1 = string.Format("{0} {1} SP20", Constants.MAIN_DC2DC_SETUP_CMD_72A_1, wiring);
        /// FETType = "72A4P40";
        fet_code = FET_T_72A_I50;
    }
    else if (pnr == 1023)
    {
        /// ELA-1023 78B
        /// A00 - 4 phases
        /// FET_TYPE:9
        /// conf:SetupDC2DC_72B50A

        /// dc2dcSetupPart1 = string.Format("{0} {1} SP20", Constants.MAIN_DC2DC_SETUP_CMD_78B50A_1, wiring);
        /// FETType = "F78B4P50A";
        fet_code = FET_T_78B_I50;
    }



	return fet_code;
}

int get_fet(int board_id ){
	int err=0;
	int fet_type  = 100;
	int fetflag = 0;
	int vpdrev ;

	pthread_mutex_lock(&i2c_mutex);
	err = setI2CSwitches(board_id);

	if (err){
		fet_type = FET_ERROR_BOARD_NA;
		goto get_out;
	}

	vpdrev =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , &err);

	if (err){
			fet_type = FET_EEPROM_DEV_ERROR;
			goto get_out;
	}

	if (vpdrev == 0xFF) {
		fet_type = FET_ERROR_BLANK_VPD;
		if (is_sp30()){
			psyslog("this is an SP30, with no VPD in main board.\nWe ASSUME it's an old ELA-2013 board, hence FET=0\n");
			fet_type = 0;
		}
		goto get_out;
	}
	else if (vpdrev < FET_SUPPORT_VPD_REV){
		fet_type = FET_ERROR_VPD_FET_NOT_SUPPORT;
		goto get_out;
	}

	fetflag =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, MAIN_BOARD_VPD_FET_FLAG_ADDR_START , &err);

	if (err || fetflag != 1)
	{
		fet_type = FET_ERROR_VPD_FET_NOT_SET;
		goto get_out;
	}

	fet_type = (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, MAIN_BOARD_VPD_FET_CODE_ADDR_START, &err);
	if (err)
	{
		fet_type = FET_ERROR_VPD_READ_ERROR;
		goto get_out;
	}


	get_out:
	resetI2CSwitches();
	pthread_mutex_unlock(&i2c_mutex);

	if (fet_type == FET_ERROR_VPD_FET_NOT_SET ||
		fet_type == FET_ERROR_VPD_FET_NOT_SUPPORT )
	{
		// read ela and stuff
		mainboard_vpd_info_t vpd = {}; // allocte, and initializero
		if (mainboard_get_vpd(board_id , &vpd) != 0)
			fet_type = FET_ERROR_VPD_READ_ERROR;

		else
			fet_type = get_fet_from_ela(&vpd);
	}

	return fet_type;
}

int get_fet_str(int board_id , char * fet){
	int err=0;
	int rc=0;
	int fetflag = 0;
	int vpdrev ;

	pthread_mutex_lock(&i2c_mutex);

	err = setI2CSwitches(board_id);

	if (err){
		rc = FET_ERROR_BOARD_NA;
		goto get_out;
	}

	vpdrev =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , &err);
	if (err){
			rc = FET_ERROR_BOARD_NA;
			goto get_out;
	}

	if (vpdrev == 0xFF) {
		fprintf(stderr,"VPD not written at all\n");
		rc  = FET_ERROR_BLANK_VPD;
		goto get_out;
	}
	else if (vpdrev < FET_SUPPORT_VPD_REV){
		fprintf(stderr,"VPD from revision %d (need at least %d to include FET information)\n", vpdrev , FET_SUPPORT_VPD_REV);
		rc = FET_ERROR_VPD_FET_NOT_SUPPORT;
		goto get_out;
	}

	fetflag =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, MAIN_BOARD_VPD_FET_FLAG_ADDR_START , &err);

	if (fetflag != 1)
	{
		fprintf(stderr,"VPD does not include FET information\n");
		rc = FET_ERROR_VPD_FET_NOT_SET;
		goto get_out;
	}

	get_out:
	resetI2CSwitches();
	pthread_mutex_unlock(&i2c_mutex);

	if (rc == 0){
		readMain_I2C_eeprom(fet , board_id , MAIN_BOARD_VPD_FET_STR_ADDR_START , MAIN_BOARD_VPD_FET_STR_ADDR_LENGTH);
	}
	return  rc;
}


int mainboard_set_vpd(  int board_id, const char * vpd){

	int rc = 0;
	int err=0;
	bool firstTimeBurn = false;

	pthread_mutex_lock(&i2c_mutex);
	err = setI2CSwitches(board_id);

	int vpdrev =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , &err);
	if (vpdrev == 0xFF){
			firstTimeBurn = true;
			i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , MAIN_VPD_REV , &err);
			usleep(10000);
			for (int a = 1 ; a < MAIN_BOARD_VPD_EEPROM_SIZE ; a++){
				i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, a , 0 , &err);
				usleep(10000);
			}
	}
	else if ( vpdrev < MAIN_VPD_REV) {
			i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , MAIN_VPD_REV , &err);
			usleep(10000);
	}

	for (int b = 0 ; b < (MAIN_BOARD_VPD_FET_STR_ADDR_START-MAIN_BOARD_VPD_SERIAL_ADDR_START); b++)
	{
		i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, b+MAIN_BOARD_VPD_SERIAL_ADDR_START  , vpd[b] , &err);
		usleep(10000);
		if(err != 0)
			break;
	}

	if (err!=0)
	{
		rc = err;
	}

	resetI2CSwitches();

	pthread_mutex_unlock(&i2c_mutex);

	return rc;

}

int readMain_I2C_eeprom (char * vpd_str , int board_id , int startAddress , int length){
	int rc = 0;
	int err=0;

	pthread_mutex_lock(&i2c_mutex);

	err = setI2CSwitches(board_id);

	if (err==0){
		for (int i = 0; i < length; i++) {
			vpd_str[i] = (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, (unsigned char)(startAddress+i), &err);
			if (err!= 0)
				break;
		}
	}

	if (err!=0)
	{
		rc = err;
	}

	resetI2CSwitches();

	pthread_mutex_unlock(&i2c_mutex);

	return rc;
}

int mainboard_get_vpd(int board_id ,  mainboard_vpd_info_t *pVpd) {
	int rc = 0;
	int err = 0;

	char vpd_str[32];

	if (pVpd == NULL ) {
		fprintf(stderr,"call mainboard_get_vpd performed without allocating sturcture first\n");
		return 1;
	}

	err = readMain_I2C_eeprom(vpd_str , board_id , 0 , 32);

	if (err)
		return err;


	strncpy(pVpd->pnr, vpd_str + MAIN_BOARD_VPD_PNR_ADDR_START, MAIN_BOARD_VPD_PNR_ADDR_LENGTH );
	pVpd->pnr[MAIN_BOARD_VPD_PNR_ADDR_LENGTH] = '\0';

	strncpy(pVpd->revision, vpd_str + MAIN_BOARD_VPD_PNRREV_ADDR_START, MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH );
	pVpd->revision[MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH] = '\0';

	strncpy(pVpd->serial, vpd_str + MAIN_BOARD_VPD_SERIAL_ADDR_START, MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH );
	pVpd->serial[MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH] = '\0';

	if (err) {
		fprintf(stderr, RED            "Failed reading %s Main Board VPD (err %d)\n" RESET,(board_id==BOARD_0)?"TOP":"BOTTOM", err);
		rc =  err;
	}
	return rc;
}

