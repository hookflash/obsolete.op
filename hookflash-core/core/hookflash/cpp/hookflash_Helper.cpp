/*
 
 Copyright (c) 2012, SMB Phone Inc.
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 
 */

#include <hookflash/internal/hookflash_Helper.h>
#include <boost/regex.hpp>


namespace hookflash
{
  typedef zsLib::String String;
  
  namespace internal
  {
    struct PrivateCountryCodePhoneNumberInfo
    {
      const char *mCountryName;
      const char *mCountry2DigitISOCode;
      const char *mCountry3DigitISOCode;
      const char *mCountryDialingPrefix;
    };
    
    static PrivateCountryCodePhoneNumberInfo gCodes[] = {
      {"United States",	"US",	"USA",	"1"},
      {"Canada",	"CA",	"CAN",	"1"},
      {"Afghanistan",	"AF",	"AFG",	"93"},
      {"Albania",	"AL",	"ALB",	"355"},
      {"Algeria",	"DZ",	"DZA",	"213"},
      {"American Samoa",	"AS",	"ASM",	"1684"},
      {"Andorra",	"AD",	"AND",	"376"},
      {"Angola",	"AO",	"AGO",	"244"},
      {"Anguilla",	"AI",	"AIA",	"1264"},
      {"Antarctica",	"AQ",	"ATA",	"672"},
      {"Antigua and Barbuda",	"AG",	"ATG",	"1268"},
      {"Argentina",	"AR",	"ARG",	"54"},
      {"Armenia",	"AM",	"ARM",	"374"},
      {"Aruba",	"AW",	"ABW",	"297"},
      {"Australia",	"AU",	"AUS",	"61"},
      {"Austria",	"AT",	"AUT",	"43"},
      {"Azerbaijan",	"AZ",	"AZE",	"994"},
      {"Bahamas",	"BS",	"BHS",	"1242"},
      {"Bahrain",	"BH",	"BHR",	"973"},
      {"Bangladesh",	"BD",	"BGD",	"880"},
      {"Barbados",	"BB",	"BRB",	"1246"},
      {"Belarus",	"BY",	"BLR",	"375"},
      {"Belgium",	"BE",	"BEL",	"32"},
      {"Belize",	"BZ",	"BLZ",	"501"},
      {"Benin",	"BJ",	"BEN",	"229"},
      {"Bermuda",	"BM",	"BMU",	"1441"},
      {"Bhutan",	"BT",	"BTN",	"975"},
      {"Bolivia",	"BO",	"BOL",	"591"},
      {"Bosnia and Herzegovina",	"BA",	"BIH",	"387"},
      {"Botswana",	"BW",	"BWA",	"267"},
      {"Brazil",	"BR",	"BRA",	"55"},
      {"British Indian Ocean Territory",	"IO",	"IOT",	""},
      {"British Virgin Islands",	"VG",	"VGB",	"1284"},
      {"Brunei",	"BN",	"BRN",	"673"},
      {"Bulgaria",	"BG",	"BGR",	"359"},
      {"Burkina Faso",	"BF",	"BFA",	"226"},
      {"Burma (Myanmar)",	"MM",	"MMR",	"95"},
      {"Burundi",	"BI",	"BDI",	"257"},
      {"Cambodia",	"KH",	"KHM",	"855"},
      {"Cameroon",	"CM",	"CMR",	"237"},
      {"Cape Verde",	"CV",	"CPV",	"238"},
      {"Cayman Islands",	"KY",	"CYM",	"1345"},
      {"Central African Republic",	"CF",	"CAF",	"236"},
      {"Chad",	"TD",	"TCD",	"235"},
      {"Chile",	"CL",	"CHL",	"56"},
      {"China",	"CN",	"CHN",	"86"},
      {"Christmas Island",	"CX",	"CXR",	"61"},
      {"Cocos (Keeling) Islands",	"CC",	"CCK",	"61"},
      {"Colombia",	"CO",	"COL",	"57"},
      {"Comoros",	"KM",	"COM",	"269"},
      {"Cook Islands",	"CK",	"COK",	"682"},
      {"Costa Rica",	"CR",	"CRC",	"506"},
      {"Croatia",	"HR",	"HRV",	"385"},
      {"Cuba",	"CU",	"CUB",	"53"},
      {"Cyprus",	"CY",	"CYP",	"357"},
      {"Czech Republic",	"CZ",	"CZE",	"420"},
      {"Democratic Republic of the Congo",	"CD",	"COD",	"243"},
      {"Denmark",	"DK",	"DNK",	"45"},
      {"Djibouti",	"DJ",	"DJI",	"253"},
      {"Dominica",	"DM",	"DMA",	"1767"},
      {"Dominican Republic",	"DO",	"DOM",	"1809"},
      {"Ecuador",	"EC",	"ECU",	"593"},
      {"Egypt",	"EG",	"EGY",	"20"},
      {"El Salvador",	"SV",	"SLV",	"503"},
      {"Equatorial Guinea",	"GQ",	"GNQ",	"240"},
      {"Eritrea",	"ER",	"ERI",	"291"},
      {"Estonia",	"EE",	"EST",	"372"},
      {"Ethiopia",	"ET",	"ETH",	"251"},
      {"Falkland Islands",	"FK",	"FLK",	"500"},
      {"Faroe Islands",	"FO",	"FRO",	"298"},
      {"Fiji",	"FJ",	"FJI",	"679"},
      {"Finland",	"FI",	"FIN",	"358"},
      {"France",	"FR",	"FRA",	"33"},
      {"French Polynesia",	"PF",	"PYF",	"689"},
      {"Gabon",	"GA",	"GAB",	"241"},
      {"Gambia",	"GM",	"GMB",	"220"},
      {"Gaza Strip",	"",	"",	"970"},
      {"Georgia",	"GE",	"GEO",	"995"},
      {"Germany",	"DE",	"DEU",	"49"},
      {"Ghana",	"GH",	"GHA",	"233"},
      {"Gibraltar",	"GI",	"GIB",	"350"},
      {"Greece",	"GR",	"GRC",	"30"},
      {"Greenland",	"GL",	"GRL",	"299"},
      {"Grenada",	"GD",	"GRD",	"1473"},
      {"Guam",	"GU",	"GUM",	"1671"},
      {"Guatemala",	"GT",	"GTM",	"502"},
      {"Guinea",	"GN",	"GIN",	"224"},
      {"Guinea-Bissau",	"GW",	"GNB",	"245"},
      {"Guyana",	"GY",	"GUY",	"592"},
      {"Haiti",	"HT",	"HTI",	"509"},
      {"Holy See (Vatican City)",	"VA",	"VAT",	"39"},
      {"Honduras",	"HN",	"HND",	"504"},
      {"Hong Kong",	"HK",	"HKG",	"852"},
      {"Hungary",	"HU",	"HUN",	"36"},
      {"Iceland",	"IS",	"IS",	"354"},
      {"India",	"IN",	"IND",	"91"},
      {"Indonesia",	"ID",	"IDN",	"62"},
      {"Iran",	"IR",	"IRN",	"98"},
      {"Iraq",	"IQ",	"IRQ",	"964"},
      {"Ireland",	"IE",	"IRL",	"353"},
      {"Isle of Man",	"IM",	"IMN",	"44"},
      {"Israel",	"IL",	"ISR",	"972"},
      {"Italy",	"IT",	"ITA",	"39"},
      {"Ivory Coast",	"CI",	"CIV",	"225"},
      {"Jamaica",	"JM",	"JAM",	"1876"},
      {"Japan",	"JP",	"JPN",	"81"},
      {"Jersey",	"JE",	"JEY",	""},
      {"Jordan",	"JO",	"JOR",	"962"},
      {"Kazakhstan",	"KZ",	"KAZ",	"7"},
      {"Kenya",	"KE",	"KEN",	"254"},
      {"Kiribati",	"KI",	"KIR",	"686"},
      //Kosovo is Serbia !!!
      //{"Kosovo",	"",	"",	"381"},
      {"Kuwait",	"KW",	"KWT",	"965"},
      {"Kyrgyzstan",	"KG",	"KGZ",	"996"},
      {"Laos",	"LA",	"LAO",	"856"},
      {"Latvia",	"LV",	"LVA",	"371"},
      {"Lebanon",	"LB",	"LBN",	"961"},
      {"Lesotho",	"LS",	"LSO",	"266"},
      {"Liberia",	"LR",	"LBR",	"231"},
      {"Libya",	"LY",	"LBY",	"218"},
      {"Liechtenstein",	"LI",	"LIE",	"423"},
      {"Lithuania",	"LT",	"LTU",	"370"},
      {"Luxembourg",	"LU",	"LUX",	"352"},
      {"Macau",	"MO",	"MAC",	"853"},
      {"Macedonia",	"MK",	"MKD",	"389"},
      {"Madagascar",	"MG",	"MDG",	"261"},
      {"Malawi",	"MW",	"MWI",	"265"},
      {"Malaysia",	"MY",	"MYS",	"60"},
      {"Maldives",	"MV",	"MDV",	"960"},
      {"Mali",	"ML",	"MLI",	"223"},
      {"Malta",	"MT",	"MLT",	"356"},
      {"Marshall Islands",	"MH",	"MHL",	"692"},
      {"Mauritania",	"MR",	"MRT",	"222"},
      {"Mauritius",	"MU",	"MUS",	"230"},
      {"Mayotte",	"YT",	"MYT",	"262"},
      {"Mexico",	"MX",	"MEX",	"52"},
      {"Micronesia",	"FM",	"FSM",	"691"},
      {"Moldova",	"MD",	"MDA",	"373"},
      {"Monaco",	"MC",	"MCO",	"377"},
      {"Mongolia",	"MN",	"MNG",	"976"},
      {"Montenegro",	"ME",	"MNE",	"382"},
      {"Montserrat",	"MS",	"MSR",	"1664"},
      {"Morocco",	"MA",	"MAR",	"212"},
      {"Mozambique",	"MZ",	"MOZ",	"258"},
      {"Namibia",	"NA",	"NAM",	"264"},
      {"Nauru",	"NR",	"NRU",	"674"},
      {"Nepal",	"NP",	"NPL",	"977"},
      {"Netherlands",	"NL",	"NLD",	"31"},
      {"Netherlands Antilles",	"AN",	"ANT",	"599"},
      {"New Caledonia",	"NC",	"NCL",	"687"},
      {"New Zealand",	"NZ",	"NZL",	"64"},
      {"Nicaragua",	"NI",	"NIC",	"505"},
      {"Niger",	"NE",	"NER",	"227"},
      {"Nigeria",	"NG",	"NGA",	"234"},
      {"Niue",	"NU",	"NIU",	"683"},
      {"Norfolk Island",	"",	"NFK",	"672"},
      {"North Korea",	"KP",	"PRK",	"850"},
      {"Northern Mariana Islands",	"MP",	"MNP",	"1670"},
      {"Norway",	"NO",	"NOR",	"47"},
      {"Oman",	"OM",	"OMN",	"968"},
      {"Pakistan",	"PK",	"PAK",	"92"},
      {"Palau",	"PW",	"PLW",	"680"},
      {"Panama",	"PA",	"PAN",	"507"},
      {"Papua New Guinea",	"PG",	"PNG",	"675"},
      {"Paraguay",	"PY",	"PRY",	"595"},
      {"Peru",	"PE",	"PER",	"51"},
      {"Philippines",	"PH",	"PHL",	"63"},
      {"Pitcairn Islands",	"PN",	"PCN",	"870"},
      {"Poland",	"PL",	"POL",	"48"},
      {"Portugal",	"PT",	"PRT",	"351"},
      {"Puerto Rico",	"PR",	"PRI",	"1"},
      {"Qatar",	"QA",	"QAT",	"974"},
      {"Republic of the Congo",	"CG",	"COG",	"242"},
      {"Romania",	"RO",	"ROU",	"40"},
      {"Russia",	"RU",	"RUS",	"7"},
      {"Rwanda",	"RW",	"RWA",	"250"},
      {"Saint Barthelemy",	"BL",	"BLM",	"590"},
      {"Saint Helena",	"SH",	"SHN",	"290"},
      {"Saint Kitts and Nevis",	"KN",	"KNA",	"1869"},
      {"Saint Lucia",	"LC",	"LCA",	"1758"},
      {"Saint Martin",	"MF",	"MAF",	"1599"},
      {"Saint Pierre and Miquelon",	"PM",	"SPM",	"508"},
      {"Saint Vincent and the Grenadines",	"VC",	"VCT",	"1784"},
      {"Samoa",	"WS",	"WSM",	"685"},
      {"San Marino",	"SM",	"SMR",	"378"},
      {"Sao Tome and Principe",	"ST",	"STP",	"239"},
      {"Saudi Arabia",	"SA",	"SAU",	"966"},
      {"Senegal",	"SN",	"SEN",	"221"},
      {"Serbia",	"RS",	"SRB",	"381"},
      {"Seychelles",	"SC",	"SYC",	"248"},
      {"Sierra Leone",	"SL",	"SLE",	"232"},
      {"Singapore",	"SG",	"SGP",	"65"},
      {"Slovakia",	"SK",	"SVK",	"421"},
      {"Slovenia",	"SI",	"SVN",	"386"},
      {"Solomon Islands",	"SB",	"SLB",	"677"},
      {"Somalia",	"SO",	"SOM",	"252"},
      {"South Africa",	"ZA",	"ZAF",	"27"},
      {"South Korea",	"KR",	"KOR",	"82"},
      {"Spain",	"ES",	"ESP",	"34"},
      {"Sri Lanka",	"LK",	"LKA",	"94"},
      {"Sudan",	"SD",	"SDN",	"249"},
      {"Suriname",	"SR",	"SUR",	"597"},
      {"Svalbard",	"SJ",	"SJM",	""},
      {"Swaziland",	"SZ",	"SWZ",	"268"},
      {"Sweden",	"SE",	"SWE",	"46"},
      {"Switzerland",	"CH",	"CHE",	"41"},
      {"Syria",	"SY",	"SYR",	"963"},
      {"Taiwan",	"TW",	"TWN",	"886"},
      {"Tajikistan",	"TJ",	"TJK",	"992"},
      {"Tanzania",	"TZ",	"TZA",	"255"},
      {"Thailand",	"TH",	"THA",	"66"},
      {"Timor-Leste",	"TL",	"TLS",	"670"},
      {"Togo",	"TG",	"TGO",	"228"},
      {"Tokelau",	"TK",	"TKL",	"690"},
      {"Tonga",	"TO",	"TON",	"676"},
      {"Trinidad and Tobago",	"TT",	"TTO",	"1868"},
      {"Tunisia",	"TN",	"TUN",	"216"},
      {"Turkey",	"TR",	"TUR",	"90"},
      {"Turkmenistan",	"TM",	"TKM",	"993"},
      {"Turks and Caicos Islands",	"TC",	"TCA",	"1649"},
      {"Tuvalu",	"TV",	"TUV",	"688"},
      {"Uganda",	"UG",	"UGA",	"256"},
      {"Ukraine",	"UA",	"UKR",	"380"},
      {"United Arab Emirates",	"AE",	"ARE",	"971"},
      {"United Kingdom",	"GB",	"GBR",	"44"},
      {"Uruguay",	"UY",	"URY",	"598"},
      {"US Virgin Islands",	"VI",	"VIR",	"1340"},
      {"Uzbekistan",	"UZ",	"UZB",	"998"},
      {"Vanuatu",	"VU",	"VUT",	"678"},
      {"Venezuela",	"VE",	"VEN",	"58"},
      {"Vietnam",	"VN",	"VNM",	"84"},
      {"Wallis and Futuna",	"WF",	"WLF",	"681"},
      {"West Bank",	"",	"",	"970"},
      {"Western Sahara",	"EH",	"ESH",	""},
      {"Yemen",	"YE",	"YEM",	"967"},
      {"Zambia",	"ZM",	"ZMB",	"260"},
      {"Zimbabwe",	"ZW",	"ZWE",	"263"},
      {NULL, NULL, NULL, NULL}
    };
    
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Helper
    #pragma mark
    
    //-------------------------------------------------------------------------
    bool Helper::normalizePhoneNumber(
                                      const String &inISOCountryCode,
                                      const String &inPhoneNumber,
                                      NormalizedPhoneNumber &outNormalizedPhoneNumber
                                      )
    {
      outNormalizedPhoneNumber.clear();

      if (inPhoneNumber.isEmpty()) return false;

      // .............. vvvv TODO: REPLACE THIS vvvv ..................
      outNormalizedPhoneNumber = inPhoneNumber;

      //strip string from any of these characters " ,.-():"
      std::string::size_type badCharPos = 0;
      while( (badCharPos = outNormalizedPhoneNumber.find_first_of(" ,.-()", badCharPos)) != outNormalizedPhoneNumber.npos )
      {
        outNormalizedPhoneNumber.erase(badCharPos, 1);
      }

      if (outNormalizedPhoneNumber.substr(0,1) == "+")
      {
        //remove "+" sign
        outNormalizedPhoneNumber=outNormalizedPhoneNumber.substr(1, outNormalizedPhoneNumber.size()-1);
        
        //if the next char is zero, return false
        if(outNormalizedPhoneNumber.substr(0,1) == "0") return false;
      }
      else
      {
        //trim first zero if exists
        if(outNormalizedPhoneNumber.substr(0,1) == "0")
        {
          outNormalizedPhoneNumber=outNormalizedPhoneNumber.substr(1, outNormalizedPhoneNumber.size()-1);
        }
        
        //append country code
        outNormalizedPhoneNumber.insert((std::string::size_type) 0, inISOCountryCode);
      }
      
      
      //validate phone number
      static const boost::regex phoneNumberRegex("[0-9]{7,15}");
      bool phoneNumberIsValid = regex_match(outNormalizedPhoneNumber, phoneNumberRegex);
      
      //append "+" sign
      outNormalizedPhoneNumber.insert((std::string::size_type) 0, "+");
      
      return phoneNumberIsValid;
      // .............. ^^^^ TODO: REPLACE THIS ^^^^ ..................
    }

    //-------------------------------------------------------------------------
    void Helper::normalizedPhoneNumberSplit(
                                            const NormalizedPhoneNumber &inNormalizedPhoneNumber,
                                            String &outCountryPrefix,
                                            String &outLocalPrefix,
                                            String &outDigits,
                                            String &outExtensionDigits
                                            )
    {
      outCountryPrefix.clear();
      outLocalPrefix.clear();
      outDigits.clear();
      outExtensionDigits.clear();

      if (inNormalizedPhoneNumber.isEmpty()) return;

      // .............. vvvv TODO: REPLACE THIS vvvv ..................
      outDigits = inNormalizedPhoneNumber;
      // .............. ^^^^ TODO: REPLACE THIS ^^^^ ..................
    }

    //-------------------------------------------------------------------------
    void Helper::getCountryCodePhoneNumberInfoList(CountryCodePhoneNumberInfoList &outList)
    {
      outList.clear();

      for (int loop = 0; NULL != gCodes[loop].mCountryName; ++loop)
      {
        PrivateCountryCodePhoneNumberInfo &info = (gCodes[loop]);
        CountryCodePhoneNumberInfo newInfo;

        newInfo.mCountryName = info.mCountryName;
        newInfo.mCountry2DigitISOCode = info.mCountry2DigitISOCode;
        newInfo.mCountry3DigitISOCode = info.mCountry3DigitISOCode;
        newInfo.mCountryDialingPrefix = info.mCountryDialingPrefix;
        outList.push_back(newInfo);
      }
    }
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IHelper
  #pragma mark
  
  //---------------------------------------------------------------------------
  bool IHelper::normalizePhoneNumber(
                                     const char *inISOCountryCode,  // http://countrycode.org/
                                     const char *inPhoneNumber,
                                     NormalizedPhoneNumber &outNormalizedPhoneNumber
                                     )
  {
    return internal::Helper::normalizePhoneNumber(inISOCountryCode ? String(inISOCountryCode) : String(), inPhoneNumber ? String(inPhoneNumber) : String(), outNormalizedPhoneNumber);
  }

  //---------------------------------------------------------------------------
  void IHelper::normalizedPhoneNumberSplit(
                                           const NormalizedPhoneNumber &inNormalizedPhoneNumber,
                                           String &outCountryPrefix,
                                           String &outLocalPrefix,
                                           String &outDigits,
                                           String &outExtensionDigits
                                           )
  {
    internal::Helper::normalizedPhoneNumberSplit(inNormalizedPhoneNumber, outCountryPrefix, outLocalPrefix, outDigits, outExtensionDigits);
  }

  //---------------------------------------------------------------------------
  void IHelper::getCountryCodePhoneNumberInfoList(CountryCodePhoneNumberInfoList &outList)
  {
    internal::Helper::getCountryCodePhoneNumberInfoList(outList);
  }
}
