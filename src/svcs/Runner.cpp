
#include "svcs/Runner.hpp"
#include "svcs/iSvc.hpp"

using namespace etfw;

iSvcRunner::iSvcRunner():
    Svc(nullptr),
    State(State_t::CREATED)
{}

iSvcRunner::iSvcRunner(iSvc* svc):
    Svc(svc),
    State(State_t::CREATED)
{}

iSvcRunner::~iSvcRunner() = default;


PassiveRunner::PassiveRunner(iSvc* svc):
    iSvcRunner(svc)
{}

iSvcRunner::RunStatus PassiveRunner::start()
{
    Svc->pre_run_init();
    return iSvcRunner::RunStatus::OK;
}

iSvcRunner::RunStatus PassiveRunner::stop()
{
    Svc->post_run_cleanup();
    return iSvcRunner::RunStatus::OK;
}
