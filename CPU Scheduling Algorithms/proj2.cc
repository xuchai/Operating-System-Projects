# include <iostream>
# include <vector>
# include <string>
# include <utility>
# include <fstream>
# include <sstream>
# include <queue>
# include <map>
# include <functional>

/* compile as: g++ -Wall p2.cc          */
/* run as: ./a.out                      */
/* given input file name: processes.txt */

using namespace std;

class Process;

typedef pair<int, Process*> mypair;
typedef map<int, mypair> mymap;

bool Compare(const mypair& p1, const mypair& p2);
void test_p_v(const vector<Process> p_v);
void test_event_map(mymap event_map);

typedef priority_queue<mypair, vector<mypair>, function<bool(mypair, mypair)> > my_pq;

# define t_cs 13;   /* define context switch time as 13ms   */
int final_t;


/* process class header */
class Process {
public:
    int p_id;               /* process id                       */
    int burst_t;            /* burst time                       */
    int num_burst;          /* num of bursts                    */
    int num_burst_left;     /* num of bursts left               */
    int IO_t;               /* I/O time                         */
    int priority;           /* process priority                 */
    int enter_q_t;          /* current enter ready_q time       */
    int enter_CPU_t;        /* current enter CPU time           */
    int total_wait_t;       /* total wait time in ready q       */
    int burst_left_t;       /* burst time left when preempted   */
    bool preempted;         /* preempted or not                 */
    int addition_num_burst; /* additional num of bursts         */

    /* constructors */
    Process();
    Process(const int& p_id, 
            const int& burst_t,
            const int& num_burst,
            const int& IO_t,
            const int& priority) : 
            p_id(p_id),
            burst_t(burst_t),
            num_burst(num_burst),
            num_burst_left(num_burst),
            IO_t(IO_t),
            priority(priority),
            enter_q_t(0),
            enter_CPU_t(0),
            total_wait_t(0),
            burst_left_t(burst_t),
            preempted(false),
            addition_num_burst(0) {}

    void reset();   /* reset process    */
};

/* default constructor for Process class */
Process::Process() {
    p_id = -1;
}

/* reset Process after one alg complete */
void Process::reset() {
    num_burst_left = num_burst;
    enter_q_t = 0;
    enter_CPU_t = 0;
    total_wait_t = 0;
    burst_left_t = burst_t;
    preempted = false;
    addition_num_burst = 0;
}

/* compare function for priority queues */
/* for PWA, use FCFS then p_id          */
bool Compare(const mypair& p1, const mypair& p2) {
    if (p1.first != p2.first) return p1.first > p2.first;
    else if (p1.second->enter_q_t != p2.second->enter_q_t)
        return p1.second->enter_q_t > p2.second->enter_q_t;
    else return p1.second->p_id > p2.second->p_id;
} 

/* read input file  */
void read_file(vector<Process>& p_v) {
    ifstream infile("processes.txt");
    string line;

    /* read input process data line by line into p_v */
    while (!getline(infile, line).eof()) {
        string tmp;
        istringstream ss(line);
        ss >> tmp;

        /* ignore comment line and blank line */
        if (tmp == "#" || tmp.empty()) continue;

        /* write processes data into tmp row line by line */
        vector<int> tmp_row;
        string s = "";
        for (unsigned int i = 0; i < tmp.size(); ++i) {
            if (tmp[i] == '|') {
                tmp_row.push_back(atoi(s.c_str()));
                s = "";
            }
            else s += tmp[i];
            if (i == tmp.size() - 1) tmp_row.push_back(atoi(s.c_str()));
        }
        /* initilize the Process class */
        Process tmp_p(tmp_row[0], tmp_row[1], tmp_row[2], tmp_row[3], tmp_row[4]);
        p_v.push_back(tmp_p);
    }
}

/* print current queue  */
void print_q(queue<Process*> q) {
    cout << "[Q";
    while (!q.empty()) {
        cout << " " << q.front()->p_id;
        q.pop();
    }
    cout << "]" << endl;
}

/* print current priority queue */
void print_pq(my_pq pq) {
    cout << "[Q";
    while (!pq.empty()) {
        cout << " " << pq.top().second->p_id;
        pq.pop();
    }
    cout << "]" << endl;
}

/* print queues based on different algs */
void print_queues(const string& alg, queue<Process*>& FCFS_q, my_pq& SRT_q, my_pq& PWA_q) {
    if (alg == "FCFS") print_q(FCFS_q);
    if (alg == "SRT") print_pq(SRT_q);
    if (alg == "PWA") print_pq(PWA_q);
}

/* check if simulator ended or not, return true if end */
bool check_end(const vector<Process>& p_v) {
    for (unsigned int i = 0; i < p_v.size(); ++i) {
        if (p_v[i].num_burst_left != 0) return false;
    }
    return true;
}

/* update PWA_q priority  */
void update_priority(vector<Process>& p_v, my_pq& pq, int t) {
    vector<mypair> tmp_pair;
    vector<int> tmp_v;    

    /* get all process id in current ready q    */
    while (!pq.empty()) {
        tmp_pair.push_back(pq.top());
        tmp_v.push_back(pq.top().second->p_id);
        pq.pop();
    }

    /* update priorities of processes in tmp_v */
    for (unsigned int i = 0; i < p_v.size(); ++i) {
        for (unsigned int j = 0; j < tmp_v.size(); ++j) {
            /* if wait time more than 3*burst time  */
            if (p_v[i].p_id == tmp_v[j]) {
                int curr_wait_t = t - p_v[i].enter_q_t;
                if (curr_wait_t > 3*p_v[i].burst_t) {
                    --p_v[i].priority;
                    --tmp_pair[j].first;
                    if (p_v[i].priority < 0) p_v[i].priority = 0;
                    if (tmp_pair[j].first < 0) tmp_pair[j].first = 0; 
                }
            }
        }
    }

    /* insert updated pairs back into ready q   */
    for (unsigned int i = 0; i < tmp_pair.size(); ++i) {
        pq.push(tmp_pair[i]);
    }
}

/* push event into event map    */
void push_event(vector<Process>& p_v,
                queue<Process*>& FCFS_q,
                my_pq& SRT_q,
                my_pq& PWA_q,
                vector<mypair>& tie_p,
                mymap& event_map,
                int t,
                const string& alg) {

    Process* curr_p = NULL;

    /* check if ready q empty or not    */
    /* if not, store front of ready q   */
    if (alg == "FCFS")  {
        if (FCFS_q.empty()) return;
        else curr_p = FCFS_q.front();
    }    
    else if (alg == "SRT") {
        if (SRT_q.empty()) return;
        else curr_p = SRT_q.top().second;
    }
    else if (alg == "PWA") {
        if (PWA_q.empty()) return;
        else curr_p = PWA_q.top().second;
    }

    /* caculate related event time  */
    int t_burst_start = t + t_cs;
    int t_burst_end;
    if (curr_p->preempted) {
        t_burst_end = t_burst_start + curr_p->burst_left_t;
        curr_p->preempted = false;
    }
    else t_burst_end = t_burst_start + curr_p->burst_t;
    int t_IO_end = t_burst_end + curr_p->IO_t;

    pair<mymap::iterator, bool> ret;

    /* insert start CPU burst time into event map   */
    ret = event_map.insert(make_pair(t_burst_start, make_pair(0, curr_p)));

    if (ret.second == false) {
        cout << "ERROR: failed insert at time " << t << endl;
        cout << "p_id: "  << curr_p->p_id << endl;
        return;
    } 

    /* insert finish burst time into event map   */
    event_map.insert(make_pair(t_burst_end, make_pair(1, curr_p)));

    if (ret.second == false) {
        cout << "ERROR: failed insert at time " << t << endl;
        cout << "p_id: "  << curr_p->p_id << endl;
        return;
    } 

    /* if I/O burst time 0 or last CPU burst, done  */
    if (curr_p->IO_t == 0 || curr_p->num_burst_left == 1) return;

    /* else, insert finish I/O time into event map  */
    /* check conflicts, if already has same time, add into tie_data */
    /* add it into tie_p                            */
    ret = event_map.insert(make_pair(t_IO_end, make_pair(2, curr_p)));
    if (ret.second == false) {
        tie_p.push_back(make_pair(t_IO_end, curr_p));
    }
}

/* execute the first event in event map, 
   trigger relative event and pop first event */
void pop_event(vector<Process>& p_v,
               queue<Process*>& FCFS_q,
               my_pq& SRT_q,
               my_pq& PWA_q,
               vector<mypair>& tie_p,
               mypair& curr_p,
               mymap& event_map,
               const string& alg) {

    /* get the first event in event map */
    mymap::iterator it = event_map.begin();
 

    /* if process starts CPU burst  */
    if (it->second.first == 0) {
        /* pop the ready queue      */
        /* set the curr_p in CPU    */
        if (alg == "FCFS") {
            curr_p = make_pair(1, FCFS_q.front());
            FCFS_q.pop();
        } 
        if (alg == "SRT") {
            curr_p = make_pair(1, SRT_q.top().second);
            SRT_q.pop();
        }
        if (alg == "PWA") {
            curr_p = make_pair(1, PWA_q.top().second);
            PWA_q.pop();
        }

        /* caculate the waiting time    */
        int curr_wait_t = it->first - it->second.second->enter_q_t - t_cs;
        it->second.second->total_wait_t += curr_wait_t;

        /* set enter CPU time */
        it->second.second->enter_CPU_t = it->first;

        /* update priority if PWA   */
        if (alg == "PWA") update_priority(p_v, PWA_q, it->first);
        
        /* output start CPU lines   */
        cout << "time " << it->first << "ms: P"
             << it->second.second->p_id << " started using the CPU ";
        print_queues(alg, FCFS_q, SRT_q, PWA_q);
    }

    /* if process completed CPU burst   */
    else if (it->second.first == 1) {
        /* update priority if PWA   */
        if (alg == "PWA") update_priority(p_v, PWA_q, it->first);
        
        /* decrease process num burst left by 1 */
        --(it->second.second->num_burst_left);
    
        /* set current process back to <0, NULL>    */
        curr_p.first = 0;
        curr_p.second = NULL;

        /* set burst left t back to burst t */
        it->second.second->burst_left_t = it->second.second->burst_t;

        /* if process terminated    */
        if (it->second.second->num_burst_left == 0) {
            cout << "time " << it->first << "ms: P"
                 << it->second.second->p_id << " terminated ";
            print_queues(alg, FCFS_q, SRT_q, PWA_q);
        }
        
        /* else perform I/O */
        else {
            cout << "time " << it->first << "ms: P"
                 << it->second.second->p_id << " completed its CPU burst ";
            print_queues(alg, FCFS_q, SRT_q, PWA_q);

            cout << "time " << it->first << "ms: P"
                 << it->second.second->p_id << " performing I/O ";
            print_queues(alg, FCFS_q, SRT_q, PWA_q);
        }

        /* trigger next CPU burst event */
        push_event(p_v, FCFS_q, SRT_q, PWA_q, tie_p, event_map, it->first, alg);
    }

    /* if process completes I/O */
    else if (it->second.first == 2) {
        bool q_empty;
        if (alg == "FCFS") q_empty = FCFS_q.empty();
        if (alg == "SRT") q_empty = SRT_q.empty();
        if (alg == "PWA") q_empty = PWA_q.empty();
        
        /* if no conflicts data */
        if (tie_p.empty()) {
            /* check preemption  */ 
            bool check_preempt = false;
            bool later_preempt = false;            

            /* push process back into ready q, PREEMPTION HERE   */
            if (alg == "FCFS") FCFS_q.push(it->second.second);
            
            /* check preemption for SRT and PWA */
            if (alg == "SRT"|| alg == "PWA") {
                /* remaining time of curr_p in CPU */
                int curr_remain_t = 0;
                if (curr_p.first == 1) {
                    curr_remain_t = curr_p.second->burst_left_t - (it->first -  curr_p.second->enter_CPU_t);
                }               
                if (curr_remain_t < 0) curr_remain_t = curr_p.second->burst_left_t;
 
                /* check remain time and burst time for SRT */
                if (alg == "SRT") {
                    /* preempt if burst time less than curr remain time */
                    if (it->second.second->burst_t < curr_remain_t) check_preempt = true;
                }
                /* else check priority for PWA  */
                else {
                    /* update priorities, 'AGING'   */
                    update_priority(p_v, PWA_q, it->first);

                    /* preempt if priority higher than curr p in CPU    */
                    if (curr_p.first == 0) {}
                    else if (it->second.second->priority < curr_p.second->priority) {
                        check_preempt = true;
                    }

                    /* forcasting, see if later it->p priority will higher than curr p   */
                    else if (it->second.second->priority == curr_p.second->priority) {
                        if (curr_remain_t > 3*(it->second.second->burst_t)) {
                            cout << "aging: P" << it->second.second->p_id << " " << it->second.second->priority - 1 << endl;
                            later_preempt = true;
                        }
                    }     
                }            

            
                /* if preempt */
                if (check_preempt || later_preempt) {
                    int later_t = 0;
                    if (later_preempt) {
                        later_t = 3*(it->second.second->burst_t) + 1;
                        --(it->second.second->priority);
                    }

                    /* if later preempt, push it->p into ready q first  */
                    if (later_preempt) {
                        PWA_q.push(make_pair(it->second.second->priority, it->second.second));   
                    }

                    /* output complete I/O lines */
                    cout << "time " << it->first << "ms: P"
                        << it->second.second->p_id << " completed I/O ";
                    print_queues(alg, FCFS_q, SRT_q, PWA_q);

                    /* remove curr_p events from event map  */
                    for (mymap::iterator itr = event_map.begin(); itr!= event_map.end(); ++itr) {
                        if (itr == event_map.begin()) continue;
                        if (itr->second.second->p_id == curr_p.second->p_id) {
                            event_map.erase(itr);
                            --itr;
                        }
                    }

                    /* push curr_p back into ready q  */
                    curr_p.second->preempted = true;
                    curr_p.second->burst_left_t = curr_remain_t - later_t;
                    ++(curr_p.second->addition_num_burst);                 
                    curr_p.second->enter_q_t = it->first + later_t;

                    /* more caculation on later preempt */
                    if (later_preempt) {
                        it->second.second->total_wait_t += later_t;
                    } 

                    it->second.second->enter_q_t = it->first + later_t;

                    if (alg == "SRT") {
                        SRT_q.push(make_pair(curr_remain_t, curr_p.second));
                    }
                    else {
                        PWA_q.push(make_pair(curr_p.second->priority, curr_p.second));
                    }

                    /* output preempted lines */
                    cout << "time " << it->first + later_t << "ms: P"
                         << curr_p.second->p_id << " preempted by P" << it->second.second->p_id <<" ";
                    /* if later preempt, change the curr PWA q  */
                    /* because top p preempt the curr p later   */
                    if (later_preempt) {
                        my_pq tmp_pq(Compare);
                        tmp_pq = PWA_q;
                        tmp_pq.pop();
                        print_queues(alg, FCFS_q, SRT_q, tmp_pq);
                    }
                    else print_queues(alg, FCFS_q, SRT_q, PWA_q);

                    /* push current event into ready queue anyway */
                    if (alg == "SRT") {
                        SRT_q.push(make_pair(it->second.second->burst_t, it->second.second));
                    }
                    else {
                        if (!later_preempt) {
                            PWA_q.push(make_pair(it->second.second->priority, it->second.second));   
                        }
                    }

                    /* push preempt event */
                    push_event(p_v, FCFS_q, SRT_q, PWA_q, tie_p, event_map, it->first+later_t, alg);
                    
                }
                 else {   /* push current event into ready queue anyway */
                    if (alg == "SRT") {
                        SRT_q.push(make_pair(it->second.second->burst_t, it->second.second));
                    }
                    else {
                        PWA_q.push(make_pair(it->second.second->priority, it->second.second));   
                    }
                }
            }

            /* if FCFS or not preempt, regular I/O */
            if (alg == "FCFS" || (!check_preempt &&!later_preempt)) {
                /* set process enter ready q time   */
                it->second.second->enter_q_t = it->first;

                /* output completed I/O lines   */
                cout << "time " << it->first << "ms: P"
                    << it->second.second->p_id << " completed I/O ";
                print_queues(alg, FCFS_q, SRT_q, PWA_q);
            }
        }

        /* else, add conflicts data back in process id order    */
        else {
            cout << "TIE DATA HERE" << endl;
        }

        /* if ready queue used to be empty, 
           and no process in CPU right now, 
           trigger the next push event */
        if (q_empty && curr_p.first == 0) {
            push_event(p_v, FCFS_q, SRT_q, PWA_q, tie_p, event_map, it->first, alg);
        }
    }
    
    /* catch the final terminated time  */
    if (check_end(p_v)) final_t = it->first;

    /* finally, pop event map  */    
    event_map.erase(it);

}

/* test output functions    */
void test_p_v(const vector<Process> p_v) {
    /* test p_v */
    cout << "test input process data :" << endl;
    cout << "p_id| burst_t| num_burst| num_burst_left| IO_t| priority| enter_q_t| total_wait_t|" << endl; 
    for (unsigned int i = 0; i < p_v.size(); ++i) {
        cout << p_v[i].p_id << " "
             << p_v[i].burst_t << " "
             << p_v[i].num_burst << " "
             << p_v[i].num_burst_left << " "
             << p_v[i].IO_t << " "
             << p_v[i].priority << " "
             << p_v[i].enter_q_t << " "
             << p_v[i].total_wait_t << " "
             << p_v[i].addition_num_burst << endl;
    }
}

/* test output functions    */
void test_event_map(mymap event_map) {
    cout << "test event map:" << endl;
    cout << "time| event| p_id " << endl;
    for (mymap::iterator itr = event_map.begin(); itr != event_map.end(); ++itr) {
        cout << itr->first << " " << itr->second.first <<" "
             << itr->second.second->p_id << endl;
    }
}

/* write alg analysis data to file simout.txt   */
void output_sim(ostream& ostr, const vector<Process>& p_v, const string& alg) {
    /* data needed to caculate  */
    int CPU_burst_t = 0;
    int wait_t = 0;
    int turnaround_t = 0;
    int num_burst = 0;
    int total_cs_count = 0;

    /* loop Process vector    */
    for (unsigned int i = 0; i < p_v.size(); ++i) {
        CPU_burst_t += p_v[i].burst_t*p_v[i].num_burst;
        wait_t += p_v[i].total_wait_t;
        turnaround_t += p_v[i].burst_t*p_v[i].num_burst + p_v[i].total_wait_t;
        num_burst += p_v[i].num_burst;
        total_cs_count += p_v[i].num_burst + p_v[i].addition_num_burst;
    }
    turnaround_t += total_cs_count*t_cs;

    /* caculation of avg time   */
    double avg_CPU_burst = double(CPU_burst_t)/num_burst;
    double avg_wait = double(wait_t)/num_burst;
    double avg_turnaround = double(turnaround_t)/num_burst;
    
    /* output results   */
    ostr.precision(2);
    ostr << fixed;
    ostr << "Algorithm " << alg << endl;
    ostr << "-- average CPU burst time: " << avg_CPU_burst << " ms" << endl;
    ostr<< "-- average wait time: " << avg_wait << " ms" << endl;
    ostr << "-- average turnaround time: " << avg_turnaround << " ms" << endl;
    ostr << "-- total number of context switches: " << total_cs_count << endl;
}

int main() {
    
    vector<Process> p_v;        /* vector of Process class  */    
    queue<Process*> FCFS_q;     /* ready queue of FCFS alg  */
    my_pq SRT_q(Compare);                /* ready queue of SRT alg   */    
    my_pq PWA_q(Compare);                /* ready queue of PWA alg   */
    mymap event_map;            /* event map, map<time, pair<event, Process*> >  */
    vector<mypair> tie_p;       /* vector of tie processes  */
    mypair curr_p;             /* current process in CPU, <1, ptr> or <0, NULL>   */

    /* output stream    */
    ofstream ostr;
    ostr.open("simout.txt");


    /* read process data from input file */
    read_file(p_v);

    /* start FCFS alg                   */
    /* push processes into ready queue  */
    for (unsigned int i = 0; i < p_v.size(); ++i) {
        Process* ptr = &p_v[i];
        FCFS_q.push(ptr);
    }
    
    /* start simulator for FCFS  */
    cout << "time 0ms: Simulator started for FCFS ";
    print_q(FCFS_q);

    /* push first event into event map  */
    push_event(p_v, FCFS_q, SRT_q, PWA_q, tie_p, event_map, 0, "FCFS");

    /* keep pop first event from map and push related events into map   */
    while (!check_end(p_v)) {
        pop_event(p_v, FCFS_q, SRT_q, PWA_q, tie_p, curr_p, event_map, "FCFS");
    }

    /* end simulator for FCFS   */
    cout << "time " << final_t << "ms: Simulator for FCFS ended ";
    print_q(FCFS_q);
    cout << endl << endl;

    //test_p_v(p_v);

    /* output simout file   */
    output_sim(ostr, p_v, "FCFS");


    /* start SRT algorithm              */
    /* reset data back to initial form  */
    event_map.clear();
    tie_p.clear();

    /* push processes into SRT queue    */
    for (unsigned int i = 0; i< p_v.size();++i) {
        /* reset each Process   */
        p_v[i].reset();
    
        Process* ptr = &p_v[i];
        SRT_q.push(make_pair(p_v[i].burst_t, ptr));
    }
    
    /* start simulator for SRT  */
    cout << "time 0ms: Simulator started for SRT ";
    print_pq(SRT_q);

    /* push first event into event map  */
    push_event(p_v, FCFS_q, SRT_q, PWA_q, tie_p, event_map, 0, "SRT");

    /* keep pop first event from map and push related events into map   */
    while (!check_end(p_v)) {
        pop_event(p_v, FCFS_q, SRT_q, PWA_q, tie_p, curr_p, event_map, "SRT");
    }

    /* end simulator for SRT    */
    cout << "time " << final_t << "ms: Simulator for SRT ended ";
    print_pq(SRT_q);
    cout << endl << endl;

    /* output simout file   */
    output_sim(ostr, p_v, "SRT");

   // test_p_v(p_v);

    /* start PWA algorithm  */
    /* reset data back to initial form  */
    event_map.clear();
    tie_p.clear();
    
    /* push process into PWA queue  */
    for (unsigned int i = 0; i < p_v.size(); ++i) {
        /* reset each Process   */
        p_v[i].reset();
        
        Process* ptr = &p_v[i];
        PWA_q.push(make_pair(p_v[i].priority, ptr));
    }

    /* start simulator for PWA  */
    cout << "time 0ms: Simulator started for PWA ";
    print_pq(PWA_q);

    /* push first event into event map  */
    push_event(p_v, FCFS_q, SRT_q, PWA_q, tie_p, event_map, 0, "PWA");
    
    /* keep pop first event from map and push related events into map   */
    while (!check_end(p_v)) {
        pop_event(p_v, FCFS_q, SRT_q, PWA_q, tie_p, curr_p, event_map, "PWA");
    }

    /* end simulator for SRT    */
    cout << "time " << final_t << "ms: Simulator for PWA ended ";
    print_pq(PWA_q);
    cout << endl << endl;

    /* output simout file   */
    output_sim(ostr, p_v, "PWA");

    ostr.close();

    return EXIT_SUCCESS;
}
