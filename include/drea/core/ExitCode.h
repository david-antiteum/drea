#pragma once

namespace drea::core {

/*! Standard process exit codes, sysexits(3)-inspired.

	Services and CLIs built on drea return the same vocabulary, so
	orchestrators and runbooks read one table. Drea itself uses
	ExitCode::ConfigError when declarative option validation fails.
*/
enum class ExitCode : int {
	Ok              = 0,	//!< success
	GeneralError    = 1,	//!< unspecified failure
	UsageError      = 64,	//!< command line used incorrectly (EX_USAGE)
	DataError       = 65,	//!< input data incorrect (EX_DATAERR)
	NoInput         = 66,	//!< input file missing or unreadable (EX_NOINPUT)
	DependencyError = 69,	//!< required service unavailable (EX_UNAVAILABLE)
	InternalError   = 70,	//!< internal software error (EX_SOFTWARE)
	OsError         = 71,	//!< operating system error (EX_OSERR)
	CantCreate      = 73,	//!< cannot create output (EX_CANTCREAT)
	IoError         = 74,	//!< input/output error (EX_IOERR)
	TempFail        = 75,	//!< temporary failure, retry may work (EX_TEMPFAIL)
	Protocol        = 76,	//!< remote error in protocol (EX_PROTOCOL)
	NoPermission    = 77,	//!< permission denied (EX_NOPERM)
	ConfigError     = 78	//!< configuration error (EX_CONFIG)
};

[[nodiscard]] constexpr int toInt( ExitCode code ) noexcept
{
	return static_cast<int>( code );
}

}
