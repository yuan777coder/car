/*********************************************************************************************************************
* STC32G144K Opensourec Library 鍗筹紙STC32G144K 寮€婧愬簱锛夋槸涓€涓熀浜庡畼鏂?SDK 鎺ュ彛鐨勭涓夋柟寮€婧愬簱
* Copyright (c) 2025 SEEKFREE 閫愰绉戞妧
*
* 鏈枃浠舵槸STC32G144K寮€婧愬簱鐨勪竴閮ㄥ垎
*
* STC32G144K 寮€婧愬簱 鏄厤璐硅蒋浠?* 鎮ㄥ彲浠ユ牴鎹嚜鐢辫蒋浠跺熀閲戜細鍙戝竷鐨?GPL锛圙NU General Public License锛屽嵆 GNU閫氱敤鍏叡璁稿彲璇侊級鐨勬潯娆?* 鍗?GPL 鐨勭3鐗堬紙鍗?GPL3.0锛夋垨锛堟偍閫夋嫨鐨勶級浠讳綍鍚庢潵鐨勭増鏈紝閲嶆柊鍙戝竷鍜?鎴栦慨鏀瑰畠
*
* 鏈紑婧愬簱鐨勫彂甯冩槸甯屾湜瀹冭兘鍙戞尌浣滅敤锛屼絾骞舵湭瀵瑰叾浣滀换浣曠殑淇濊瘉
* 鐢氳嚦娌℃湁闅愬惈鐨勯€傞攢鎬ф垨閫傚悎鐗瑰畾鐢ㄩ€旂殑淇濊瘉
* 鏇村缁嗚妭璇峰弬瑙?GPL
*
* 鎮ㄥ簲璇ュ湪鏀跺埌鏈紑婧愬簱鐨勫悓鏃舵敹鍒颁竴浠?GPL 鐨勫壇鏈?* 濡傛灉娌℃湁锛岃鍙傞槄<https://www.gnu.org/licenses/>
*
* 棰濆娉ㄦ槑锛?* 鏈紑婧愬簱浣跨敤 GPL3.0 寮€婧愯鍙瘉鍗忚 浠ヤ笂璁稿彲鐢虫槑涓鸿瘧鏂囩増鏈?* 璁稿彲鐢虫槑鑻辨枃鐗堝湪 libraries/doc 鏂囦欢澶逛笅鐨?GPL3_permission_statement.txt 鏂囦欢涓?* 璁稿彲璇佸壇鏈湪 libraries 鏂囦欢澶逛笅 鍗宠鏂囦欢澶逛笅鐨?LICENSE 鏂囦欢
* 娆㈣繋鍚勪綅浣跨敤骞朵紶鎾湰绋嬪簭 浣嗕慨鏀瑰唴瀹规椂蹇呴』淇濈暀閫愰绉戞妧鐨勭増鏉冨０鏄庯紙鍗虫湰澹版槑锛?*
* 鏂囦欢鍚嶇О          
* 鍏徃鍚嶇О          鎴愰兘閫愰绉戞妧鏈夐檺鍏徃
* 鐗堟湰淇℃伅          鏌ョ湅 libraries/doc 鏂囦欢澶瑰唴 version 鏂囦欢 鐗堟湰璇存槑
* 寮€鍙戠幆澧?         MDK FOR C251
* 閫傜敤骞冲彴          STC32G144K
* 搴楅摵閾炬帴          https://seekfree.taobao.com/
*
* 淇敼璁板綍
* 鏃ユ湡              浣滆€?          澶囨敞
* 2025-11-20        澶            first version
********************************************************************************************************************/

#include "zf_common_headfile.h"

// *************************** 渚嬬▼纭欢杩炴帴璇存槑 ***************************
// 浣跨敤 type-c 杩炴帴
//      鐩存帴灏唗ype-c鎻掑叆鏍稿績鏉匡紝鍗冲彲浣跨敤USB-CDC

// *************************** 渚嬬▼娴嬭瘯璇存槑 ***************************
// 1.鏍稿績鏉跨儳褰曞畬鎴愭湰渚嬬▼锛屼娇鐢?type-c鎺ュ彛锛屽湪鏂數鎯呭喌涓嬪畬鎴愯繛鎺?//
// 2.灏?type-c鎺ュ彛杩炴帴 鐢佃剳锛屽畬鎴愪笂鐢?//
// 3.鐢佃剳涓婁娇鐢ㄤ覆鍙ｅ姪鎵嬫墦寮€瀵瑰簲鐨勪覆鍙ｏ紝涓插彛娉㈢壒鐜囦负 zf_common_debug.h 鏂囦欢涓?DEBUG_UART_BAUDRATE 瀹忓畾涔?榛樿 115200锛屾牳蹇冩澘鎸変笅澶嶄綅鎸夐敭
//
// 4.鍙互鍦ㄤ覆鍙ｅ姪鎵嬩笂鐪嬪埌濡備笅涓插彛淇℃伅锛?//      ADC channel 1 convert data is x.
//      ...
//      ADC channel 1 mean filter convert data is x.
//      ...
//
// 5.灏?ADC_CHANNELx 瀹忓畾涔夊搴旂殑寮曡剼鍒嗗埆鎺ュ埌 3V3/GND 鍐嶅搴旂殑淇℃伅浼氱湅鍒版暟鎹彉鍖?//
// 濡傛灉鍙戠幇鐜拌薄涓庤鏄庝弗閲嶄笉绗?璇峰弬鐓ф湰鏂囦欢鏈€涓嬫柟 渚嬬▼甯歌闂璇存槑 杩涜鎺掓煡

// **************************** 浠ｇ爜鍖哄煙 ****************************
#define CHANNEL_NUMBER          ( 8 )

#define ADC_CHANNEL1            ( ADC2_CH0_P00 )
#define ADC_CHANNEL2            ( ADC2_CH1_P01 )
#define ADC_CHANNEL3            ( ADC2_CH2_P02 )
#define ADC_CHANNEL4            ( ADC2_CH3_P03 )
#define ADC_CHANNEL5            ( ADC2_CH6_P06 )
#define ADC_CHANNEL6            ( ADC2_CH7_P07 )
#define ADC_CHANNEL7            ( ADC1_CH1_P11 )
#define ADC_CHANNEL8            ( ADC1_CH4_P14 )


uint8 channel_index = 0;
adc_channel_enum channel_list[CHANNEL_NUMBER] =
{
    ADC_CHANNEL1, 
    ADC_CHANNEL2, 
    ADC_CHANNEL3, 
    ADC_CHANNEL4,
    ADC_CHANNEL5,
    ADC_CHANNEL6,
    ADC_CHANNEL7,
    ADC_CHANNEL8,
};


void main(void)
{
    clock_init(SYSTEM_CLOCK_96M); 				// 鏃堕挓閰嶇疆鍙婄郴缁熷垵濮嬪寲<鍔″繀淇濈暀>
    debug_init();                       		// 璋冭瘯涓插彛淇℃伅鍒濆鍖?
    // 姝ゅ缂栧啓鐢ㄦ埛浠ｇ爜 渚嬪澶栬鍒濆鍖栦唬鐮佺瓑
    adc_init(ADC_CHANNEL1, ADC_12BIT);          // 鍒濆鍖栧搴?ADC 閫氶亾涓哄搴旂簿搴?    adc_init(ADC_CHANNEL2, ADC_12BIT);          // 鍒濆鍖栧搴?ADC 閫氶亾涓哄搴旂簿搴?    adc_init(ADC_CHANNEL3, ADC_10BIT);          // 鍒濆鍖栧搴?ADC 閫氶亾涓哄搴旂簿搴?    adc_init(ADC_CHANNEL4, ADC_8BIT);           // 鍒濆鍖栧搴?ADC 閫氶亾涓哄搴旂簿搴?    adc_init(ADC_CHANNEL5, ADC_8BIT);           // 鍒濆鍖栧搴?ADC 閫氶亾涓哄搴旂簿搴?    adc_init(ADC_CHANNEL6, ADC_8BIT);           // 鍒濆鍖栧搴?ADC 閫氶亾涓哄搴旂簿搴?    adc_init(ADC_CHANNEL7, ADC_8BIT);           // 鍒濆鍖栧搴?ADC 閫氶亾涓哄搴旂簿搴?    adc_init(ADC_CHANNEL8, ADC_8BIT);           // 鍒濆鍖栧搴?ADC 閫氶亾涓哄搴旂簿搴?
                                                // 杩欓噷浣犱細鍙戠幇杈撳嚭鍏ㄦ槸 8bit 绮惧害
                                                // 鍥犱负杩欓兘鏄悓涓€涓?ADC 妯″潡鐨勫紩鑴?                                                // 鎵€浠ヤ細浠ユ渶鍚庝竴涓垵濮嬪寲绮惧害涓哄噯
    // 姝ゅ缂栧啓鐢ㄦ埛浠ｇ爜 渚嬪澶栬鍒濆鍖栦唬鐮佺瓑

    
    while(1)
    {
        // 姝ゅ缂栧啓闇€瑕佸惊鐜墽琛岀殑浠ｇ爜
        for(channel_index = 0; channel_index < CHANNEL_NUMBER; channel_index ++)
        {
            printf(
                "ADC channel %d convert data is %d.\r\n",
                channel_index + 1,
                adc_convert(channel_list[channel_index]));                      // 寰幆杈撳嚭鍗曟杞崲缁撴灉
        }
        system_delay_ms(500);

        for(channel_index = 0; channel_index < CHANNEL_NUMBER; channel_index ++)
        {
            printf(
                "ADC channel %d mean filter convert data is %d.\r\n",
                channel_index + 1,
                adc_mean_filter_convert(channel_list[channel_index], 10));      // 寰幆杈撳嚭 10 娆″潎鍊兼护娉㈣浆鎹㈢粨鏋?        }
        system_delay_ms(500);
        // 姝ゅ缂栧啓闇€瑕佸惊鐜墽琛岀殑浠ｇ爜
    }
}
// **************************** 浠ｇ爜鍖哄煙 ****************************

// *************************** 渚嬬▼甯歌闂璇存槑 ***************************
// 閬囧埌闂鏃惰鎸夌収浠ヤ笅闂妫€鏌ュ垪琛ㄦ鏌?//
// 闂1锛氫覆鍙ｆ病鏈夋暟鎹?//      鏌ョ湅涓插彛鍔╂墜鎵撳紑鐨勬槸鍚︽槸姝ｇ‘鐨勪覆鍙ｏ紝妫€鏌ユ墦寮€鐨?COM 鍙ｆ槸鍚﹀搴旂殑type-c鐨刄SB_CDC
//      濡傛灉鏄殑type-c鐨刄SB_CDC杩炴帴锛岄偅涔堟鏌ヤ笅杞藉櫒绾挎槸鍚︽澗鍔?//
// 闂2锛氫覆鍙ｆ暟鎹贡鐮?//      鏌ョ湅涓插彛鍔╂墜璁剧疆鐨勬尝鐗圭巼鏄惁涓庣▼搴忚缃竴鑷达紝绋嬪簭涓?zf_common_debug.h 鏂囦欢涓?DEBUG_UART_BAUDRATE 瀹忓畾涔変负 debug uart 浣跨敤鐨勪覆鍙ｆ尝鐗圭巼
