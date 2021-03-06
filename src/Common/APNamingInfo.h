//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef APNAMINGINFO_H_
#define APNAMINGINFO_H_

//Standard libraries
#include <string>
#include <sstream>

//RINASim libraries
#include "APN.h"

/**
 * @brief APNamingInfo holds complete naming info for particular application process.
 *
 * APNI contains for internal properties: APN, AP-instance id,
 * AE name, AE-instance id. Only the first one is mandatory, the rest
 * is optional.
 *
 * @authors Vladimir Vesely (ivesely@fit.vutbr.cz)
 * @date Last refactorized and documented on 2014-10-28
 */
class APNamingInfo
{
  public:
    /**
     * @brief Constructor of blank APNI
     */
    APNamingInfo();

    /**
     * @brief Constructor of APNI with only APN initialized
     * @param napn New APN
     */
    APNamingInfo(APN napn);

    /**
     * @brief Construcor of fully initialized APNI
     * @param napn New APN
     * @param napinstance New AP instance identifier
     * @param naename New AE identifier
     * @param naeinstance New AE instance identifier
     */
    APNamingInfo(APN napn, std::string napinstance, std::string naename, std::string naeinstance);

    /**
     * @brief Destructor assigning uninitialized values to APNI.
     */
    virtual ~APNamingInfo();

    /**
     * @brief Equal operator overload
     * @param other Other APNI to which this one is being compared
     * @return Returns true if all APN, AP-instance id, AE name and AE-instance id
     *         are equl. Otherwise returns false.
     */
    bool operator== (const APNamingInfo& other) const
    {
        return (apn == other.apn &&
                !apinstance.compare(other.apinstance) &&
                !aename.compare(other.aename) && !aeinstance.compare(other.aeinstance) );
    }

    /**
     * @brief Info text output suitable for << string streams and  WATCH
     * @return APNI string representation
     */
    std::string info() const;

    /**
     * @brief Getter of AE-instance attribute
     * @return AE-instance id value
     */
    const std::string& getAeinstance() const {
        return aeinstance;
    }

    /**
     * @brief Setter of AE-instance attribute
     * @param aeinstance A new AE-instance id value
     */
    void setAeinstance(const std::string& aeinstance) {
        this->aeinstance = aeinstance;
    }

    /**
     * @brief Getter of AE name
     * @return AE name value
     */
    const std::string& getAename() const {
        return aename;
    }

    /**
     * @brief Setter of AE name attribute
     * @param aename A new AE name value
     */
    void setAename(const std::string& aename) {
        this->aename = aename;
    }

    /**
     * @brief Getter of AP-instance id
     * @return AP-instance id value
     */
    const std::string& getApinstance() const {
        return apinstance;
    }

    /**
     * @brief Setter of AP-instance id
     * @param apinstance A new AP-instance id value
     */
    void setApinstance(const std::string& apinstance) {
        this->apinstance = apinstance;
    }

    /**
     * @brief Getter of APN
     * @return APN
     */
    const APN& getApn() const {
        return apn;
    }

    /**
     * @brief Setter of APN
     * @param apn A new APN value
     */
    void setApn(const APN& apn) {
        this->apn = apn;
    }

  protected:
    /**
     * @brief Mandatory APN
     */
    APN apn;

    /**
     * @brief Optional AP-instance id
     */
    std::string apinstance;

    /**
     * @brief Optional AE name
     */
    std::string aename;

    /**
     * @brief Optional AE-instance id
     */
    std::string aeinstance;
};

/**
 * @brief APNamingInfo is subclassed by APNI for purely estetic purposes
 */
class APNI: public APNamingInfo {};

//Free function
/**
 * @brief << operator overload that calls APNI.info() method
 * @param os Resulting ostream
 * @param apni APNI class that is being converted to string
 * @return Infotext representing APNI
 */
std::ostream& operator<< (std::ostream& os, const APNamingInfo& apni);

#endif /* APNAMINGINFO_H_ */
