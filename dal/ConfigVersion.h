#ifndef CONFIGVERSION_H
#define CONFIGVERSION_H

#include <is/info.h>

#include <string>
#include <ostream>


// <<BeginUserCode>>

// <<EndUserCode>>
/**
 * The class is used to store GIT version of the OKS database used for given partition.
 * 
 * @author  produced by the IS generator
 */

class ConfigVersion : public ISInfo {
public:

    /**
     * OKS GIT SHA key used for given partition
     */
    std::string                   Version;


    static const ISType & type() {
	static const ISType type_ = ConfigVersion( ).ISInfo::type();
	return type_;
    }

    std::ostream & print( std::ostream & out ) const {
	ISInfo::print( out );
	out << std::endl;
	out << "Version: " << Version << "\t// OKS GIT SHA key used for given partition";
	return out;
    }

    ConfigVersion( )
      : ISInfo( "ConfigVersion" )
    {
	initialize();
    }

    ~ConfigVersion(){

// <<BeginUserCode>>

// <<EndUserCode>>
    }

protected:
    ConfigVersion( const std::string & type )
      : ISInfo( type )
    {
	initialize();
    }

    void publishGuts( ISostream & out ){
	out << Version;
    }

    void refreshGuts( ISistream & in ){
	in >> Version;
    }

private:
    void initialize()
    {

// <<BeginUserCode>>

// <<EndUserCode>>
    }


// <<BeginUserCode>>

// <<EndUserCode>>
};

// <<BeginUserCode>>

// <<EndUserCode>>
inline std::ostream & operator<<( std::ostream & out, const ConfigVersion & info ) {
    info.print( out );
    return out;
}

#endif // CONFIGVERSION_H
