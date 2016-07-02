/** Code shared among various applications
 *
 * This is a private header file outside of any namespaces, beware!
 *
 *  @file
 *  @date 5/31/16
 *  @version 1.0
 *  @copyright GNU Public License.
 */
#pragma once
#include "interop/model/run_metrics.h"


/** Exit codes that can be produced by the application
 */
enum exit_codes
{
    /** The program exited cleanly, 0 */
    SUCCESS,
    /** Invalid arguments were given to the application*/
    INVALID_ARGUMENTS,
    /** InterOp file has a bad format */
    BAD_FORMAT,
    /** Unknown error has occurred*/
    UNEXPECTED_EXCEPTION,
    /** InterOp file has not records */
    EMPTY_INTEROP,
    /** RunInfo is missing */
    MISSING_RUNINFO_XML,
    /** XML is malformed */
    MALFORMED_XML
};

/** Read run metrics from the given filename
 *
 * This function handles many error conditions.
 *
 * @param filename run folder containing RunInfo.xml and InterOps
 * @param metrics run metrics
 * @return exit code
 */
inline int read_run_metrics(const char* filename, illumina::interop::model::metrics::run_metrics& metrics)
{
    using namespace illumina::interop;
    using namespace illumina::interop::model;
    try
    {
        metrics.read(filename);
    }
    catch(const model::index_out_of_bounds_exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return UNEXPECTED_EXCEPTION;
    }
    catch(const xml::xml_file_not_found_exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return MISSING_RUNINFO_XML;
    }
    catch(const xml::xml_parse_exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return MALFORMED_XML;
    }
    catch(const io::bad_format_exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return BAD_FORMAT;
    }
    catch(const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return UNEXPECTED_EXCEPTION;
    }
    if(metrics.empty())
    {
        std::cerr << "No InterOp files found" << std::endl;
        return EMPTY_INTEROP;
    }
    return SUCCESS;
}
