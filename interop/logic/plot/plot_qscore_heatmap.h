/** Plot the Q-score heat map
 *
 *  @file
 *  @date 5/5/16
 *  @version 1.0
 *  @copyright GNU Public License.
 */
#pragma once

#include "interop/util/statistics.h"
#include "interop/constants/enums.h"
#include "interop/model/model_exceptions.h"
#include "interop/model/run_metrics.h"
#include "interop/model/plot/filter_options.h"
#include "interop/model/plot/series.h"
#include "interop/model/plot/bar_point.h"
#include "interop/model/plot/heatmap_data.h"
#include "interop/logic/metric/q_metric.h"

namespace illumina { namespace interop { namespace logic { namespace plot {


    /** Populate the q-score heat map based on the filter options
     *
     * @param beg iterator to start of q-metric collection
     * @param end iterator to end of q-metric collection
     * @param bins q-score bins
     * @param options filter for metric records
     * @param data q-score heatmap
     */
    template<typename I, typename B>
    void populate_heatmap_from_compressed(I beg,
                                          I end,
                                          const std::vector<B>& bins,
                                          const model::plot::filter_options &options,
                                          model::plot::heatmap_data& data)
    {
        for (;beg != end;++beg)
        {
            if( !options.valid_tile(*beg) ) continue;
            for(size_t bin =0;bin < bins.size();++bin)
                data(beg->cycle()-1, bins[bin].value()-1) += beg->qscore_hist(bin);
        }
    }
    /** Populate the q-score heatmap based on the filter options
     *
     * @param beg iterator to start of q-metric collection
     * @param end iterator to end of q-metric collection
     * @param options filter for metric records
     * @param data q-score heatmap
     */
    template<typename I>
    void populate_heatmap_from_uncompressed(I beg,
                                            I end,
                                            const model::plot::filter_options &options,
                                            model::plot::heatmap_data& data)
    {
        for (;beg != end;++beg)
        {
            if( !options.valid_tile(*beg) ) continue;

            for(size_t bin =0;bin < beg->size();++bin)
                data(beg->cycle()-1, bin) += beg->qscore_hist(bin);
        }
    }
    /** Normalize the heat map to a percent
     *
     * @param data output heat map data
     */
    inline void normalize_heatmap(model::plot::heatmap_data& data)
    {
        float max_value = 0;
        for(size_t r=0;r<data.row_count();++r)
            for(size_t c=0;c<data.column_count();++c)
                max_value = std::max(max_value, data(r,c));
        for(size_t r=0;r<data.row_count();++r)
            for(size_t c=0;c<data.column_count();++c)
                data(r,c) = 100 * data(r,c) / max_value;
    }
    /** Spread the bins out
     *
     *
     * @param beg iterator to start of bins
     * @param end iterator to end of bins
     * @param max_cycle maximum cycle number
     * @param data output heat map data
     */
    template<typename I>
    void remap_to_bins(I beg, I end, const size_t max_cycle, model::plot::heatmap_data& data)
    {
        for(;beg != end;++beg)
        {
            for(size_t bin = std::max(0, beg->lower()-1), upper=beg->upper();bin < upper;++bin)
            {
                for(size_t cycle = 0;cycle < max_cycle;++cycle)
                {
                    data(cycle, bin) = data(cycle, beg->value()-1);
                }
            }
        }
    }
    /** Plot a heat map of q-scores
     *
     * @param metrics q-metrics (full or by lane)
     * @param options options to filter the data
     * @param data output heat map datall
     */
    template<class Metric>
    void populate_heatmap(const model::metric_base::metric_set<Metric>& metric_set,
                          const model::plot::filter_options& options,
                          model::plot::heatmap_data& data) throw(model::index_out_of_bounds_exception)
    {
        const size_t max_q_val = logic::metric::max_qval(metric_set);
        const size_t max_cycle = metric_set.max_cycle();
        data.resize(max_cycle, max_q_val);
        INTEROP_ASSERT(data.row_count() > 0);
        INTEROP_ASSERTMSG(data.column_count() > 0, max_q_val << ", " << metric_set.size() << ", " << metric_set.bin_count() << ", " << metric::is_compressed(metric_set) << ", " << metric_set.bins().back().upper());
        const bool is_compressed = logic::metric::is_compressed(metric_set);
        if(is_compressed)
            populate_heatmap_from_compressed(metric_set.begin(),
                                             metric_set.end(),
                                             metric_set.bins(),
                                             options,
                                             data);
        else
            populate_heatmap_from_uncompressed(metric_set.begin(),
                                               metric_set.end(),
                                               options,
                                               data);
        normalize_heatmap(data);
        remap_to_bins(metric_set.bins().begin(),
                      metric_set.bins().end(),
                      max_cycle,
                      data);
    }
    /** Plot a heat map of q-scores
     *
     * @ingroup plot_logic
     * @param metrics run metrics
     * @param options options to filter the data
     * @param data output heat map data
     */
    inline void plot_qscore_heatmap(model::metrics::run_metrics& metrics,
                                    const model::plot::filter_options& options,
                                    model::plot::heatmap_data& data)
    {
        data.clear();
        if(options.is_specific_surface())
        {
            typedef model::metrics::q_metric metric_t;
            if (metrics.get_set<metric_t>().size() == 0)return;
            populate_heatmap(metrics.get_set<metric_t>(), options, data);
        }
        else
        {
            typedef model::metrics::q_by_lane_metric metric_t;
            if(0 == metrics.get_set<metric_t>().size())
                logic::metric::create_q_metrics_by_lane(metrics.get_set<model::metrics::q_metric>(),
                                                        metrics.get_set<metric_t>());
            if (metrics.get_set<metric_t>().size() == 0)return;
            populate_heatmap(metrics.get_set<metric_t>(), options, data);
        }

        data.set_xrange(0, static_cast<float>(data.row_count()));
        data.set_yrange(0, static_cast<float>(data.column_count()));

        data.set_xlabel("Cycle");
        data.set_ylabel("Q Score");

        std::string title = metrics.run_info().flowcell().barcode();
        if(title != "") title += " ";
        title += options.lane_description();
        if(metrics.run_info().flowcell().surface_count()>1 && options.is_specific_surface())
            title += " " + options.surface_description();
        data.set_title(title);
    }


}}}}
