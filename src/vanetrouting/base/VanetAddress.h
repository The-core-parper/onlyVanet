//
// Copyright (C) 2012 OpenSim Ltd
// Copyright (C) 2015 Joanne Skiles
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, see <http://www.gnu.org/licenses/>.
//



#ifndef __INET_VANETADDRESS_H
#define __INET_VANETADDRESS_H

#include "IPv4Address.h"
#include "IPv6Address.h"
#include "IPvXAddress.h"
#include "MACAddress.h"

/**
 * Stores an IPv4, IPv6 or an MACAddress address. This class should be used in
 * vanetrouting, to guarantee IPv4/IPv6/MACAddress transparency.
 *
 * Storage is efficient: an object occupies size of an IPv6 address
 * (128 bits=16 bytes) plus a short int (address type).
 */
// TODO: merge into Address
class VanetAddress
{
  public:
    enum AddrType
    {
        UNDEFINED = 0,
        IPv4_ADDRESS,
        IPv6_ADDRESS,
        MAC_ADDRESS
    };

    static VanetAddress ZERO;
    /// Constructors
    VanetAddress() { addrType = UNDEFINED; hi = 0; lo = 0; }
    VanetAddress(const VanetAddress& o) : hi(o.hi), lo(o.lo), addrType(o.addrType) {}
    explicit VanetAddress(IPv4Address addr) { set(addr); }
    explicit VanetAddress(const IPv6Address& addr) { set(addr); }
    explicit VanetAddress(const IPvXAddress& addr) { set(addr); }
    explicit VanetAddress(MACAddress addr) { set(addr); }

    /// Setters
    void set(const IPv4Address& addr) { addrType = IPv4_ADDRESS; hi = addr.getInt(); lo = 0; }
    void set(const IPv6Address& addr);
    void set(const IPvXAddress& addr);
    void set(const MACAddress& addr) { addrType = MAC_ADDRESS; hi = addr.getInt(); lo = 0; }

    /// Getters
    IPv4Address getIPv4() const;
    IPv6Address getIPv6() const;
    IPvXAddress getIPvX() const;
    MACAddress getMAC() const;

    /// Get address type
    AddrType getType() const { return (AddrType)addrType; }

    /// Set prefix: masking address with given mask. The mask specified by masklen
    void setPrefix(short unsigned int masklen);

    /// Returns string representation
    std::string str() const;

    /**
     * Compare this and other and returns:
     * -1 if this < other
     *  0 if this == other
     *  1 if this > other
     */
    short int compare(const VanetAddress& other) const;

    /// Compare operators
    bool operator ==(const VanetAddress& other) const { return addrType==other.addrType && hi==other.hi && lo==other.lo; }
    bool operator !=(const VanetAddress& other) const { return !operator==(other); }
    bool operator <(const VanetAddress& other) const { return compare(other) < 0; }
    bool operator <=(const VanetAddress& other) const { return compare(other) <= 0; }
    bool operator >(const VanetAddress& other) const { return compare(other) > 0; }
    bool operator >=(const VanetAddress& other) const { return compare(other) >= 0; }

    /**
     * Returns true if this is the broadcast address.
     */
    bool isBroadcast() const;

    /**
     * Returns true if this is a multicast logical address.
     */
    bool isMulticast() const;

    /**
     * Returns true if all address bytes are zero.
     */
    bool isUnspecified() const;

  protected:
    /// helper functions
    IPv4Address _getIPv4() const { return IPv4Address(hi); }
    IPv6Address _getIPv6() const { return IPv6Address(hi>>32, hi, lo>>32, lo); }
    MACAddress  _getMAC() const  { return MACAddress(hi); }

  protected:
    /// Member variables:
    uint64_t hi;
    uint64_t lo;
    short int addrType;     // AddrType
};

/*
 * Stores an IPv4, IPv6 or an MACAddress address with prefix. This class should be used in
 * vanetrouting, to guarantee IPv4/IPv6/MACAddress transparency.
 *
 * Storage is efficient: an object occupies size of an IPv6 address
 * (128 bits=16 bytes) plus two short int (address type and prefix length).
 */
// TODO: move into Address.h and rename to AddressPrefix
class VanetNetworkAddress
{
  public:
    VanetNetworkAddress() : prefixLength(0) {}
    VanetNetworkAddress(const VanetNetworkAddress& o) : address(o.address), prefixLength(o.prefixLength) {}
    VanetNetworkAddress(const VanetAddress& o, short unsigned int masklen) : address(o), prefixLength(masklen) { setPrefixLen(masklen); }
    explicit VanetNetworkAddress(IPv4Address addr, short unsigned int masklen=32) { set(addr, masklen); }
    explicit VanetNetworkAddress(const IPv6Address& addr, short unsigned int masklen=128) { set(addr, masklen); }
    explicit VanetNetworkAddress(const IPvXAddress& addr) { set(addr); }
    VanetNetworkAddress(const IPvXAddress& addr, short unsigned int masklen) { set(addr, masklen); }
    explicit VanetNetworkAddress(MACAddress addr, short unsigned int masklen=48) { set(addr, masklen); }

    void set(IPv4Address addr, short unsigned int masklen = 32);
    void set(const IPv6Address& addr, short unsigned int masklen = 128);
    void set(const IPvXAddress& addr);
    void set(const IPvXAddress& addr, short unsigned int masklen);
    void set(MACAddress addr, short unsigned int masklen = 48);

    void setPrefixLen(short unsigned int masklen);

    const VanetAddress& getAddress() const { return address; }
    short unsigned int getPrefixLength() const { return prefixLength; }
    VanetAddress::AddrType getType() const { return address.getType(); }

    std::string str() const;

    /**
     * compare this and o and returns:
     * -1 if this < other
     *  0 if this == other
     *  1 if this > other
     */
    short int compare(const VanetNetworkAddress& other) const;

    bool operator ==(const VanetNetworkAddress& other) const { return prefixLength==other.prefixLength && address==other.address; }
    bool operator !=(const VanetNetworkAddress& other) const { return !operator==(other); }
    bool operator <(const VanetNetworkAddress& other) const { return compare(other) < 0; }
    bool operator <=(const VanetNetworkAddress& other) const { return compare(other) <= 0; }
    bool operator >(const VanetNetworkAddress& other) const { return compare(other) > 0; }
    bool operator >=(const VanetNetworkAddress& other) const { return compare(other) >= 0; }

    bool contains(const VanetAddress& other) const;
    bool contains(const VanetNetworkAddress& other) const;

  protected:
    // member variables:
    VanetAddress address;
    short unsigned int prefixLength;
};

inline std::ostream& operator<<(std::ostream& os, const VanetAddress& addr)
{
    return os << addr.str();
}

inline std::ostream& operator<<(std::ostream& os, const VanetNetworkAddress& addr)
{
    return os << addr.str();
}


#endif  // __INET_VANETADDRESS_H

