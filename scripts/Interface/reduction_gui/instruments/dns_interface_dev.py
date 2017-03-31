class DNSInterface(InstrumentInterface):

    def __init__(self, name, settings):
	InstrumentInterface.__init(self, name, settings)

	self.ERROR_REPORT_NAME = 'dns_error_report.xml'
	self.LAST_REDUCTION_NAME = '.mantid_last_dns_reduction.xml'

	self.scripter = DNSReductionScripter(name, settings.facility_name)
	self.attach(DNSSetupWidget(settings))
