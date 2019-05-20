
#include "PipelineConstants.h"

#include "Enum.h"

const std::vector<PIPELINE> PIPELINE_ALL = {
    PIPELINE::TRI_LIST_COLOR,    //
    PIPELINE::LINE,              //
    PIPELINE::TRI_LIST_TEX,      //
    PIPELINE::CUBE,              //
    PIPELINE::PBR_COLOR,         //
    PIPELINE::PBR_TEX,           //
    PIPELINE::BP_TEX_CULL_NONE,  //
    PIPELINE::PARALLAX_SIMPLE,   //
    PIPELINE::PARALLAX_STEEP,    //
};

const std::map<VERTEX, std::set<PIPELINE>> VERTEX_PIPELINE_MAP = {
    {
        VERTEX::COLOR,
        {

            PIPELINE::TRI_LIST_COLOR,
            PIPELINE::LINE,
            PIPELINE::PBR_COLOR,
            PIPELINE::CUBE,
        },
    },
    {
        VERTEX::TEXTURE,
        {
            PIPELINE::TRI_LIST_TEX,
            PIPELINE::PBR_TEX,
            PIPELINE::BP_TEX_CULL_NONE,
            PIPELINE::PARALLAX_SIMPLE,
            PIPELINE::PARALLAX_STEEP,
        },
    },
};
