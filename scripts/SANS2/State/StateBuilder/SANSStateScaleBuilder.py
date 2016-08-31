# pylint: disable=too-few-public-methods

import copy
from SANS2.State.SANSStateScale import (SANSStateScaleISIS)
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)
from SANS2.Common.SANSEnumerations import SANSInstrument


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateScaleISISBuilder(object):
    @automatic_setters(SANSStateScaleISIS, exclusions=[])
    def __init__(self):
        super(SANSStateScaleISISBuilder, self).__init__()
        self.state = SANSStateScaleISIS()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateScaleBuilder
# ------------------------------------------
def get_scale_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return SANSStateScaleISISBuilder()
    else:
        raise NotImplementedError("SANSStateScaleBuilder: Could not find any valid scale builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
