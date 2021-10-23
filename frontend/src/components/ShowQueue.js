import React,{useState,useEffect} from 'react'
import './ShowQueue.css'
import {  toast } from 'react-toastify';
import 'react-toastify/dist/ReactToastify.css';
function ShowQueue(props) {
    toast.configure();
    let[disp_q, setdisp_q] = useState('--');
    let[disp_r, setdisp_r] = useState('--');
    useEffect(()=>{
        if((disp_r<5 && disp_r>0) && (disp_q !== '--'))
        {  
            toast.warn(disp_r.toString()+' queues remain ',{position: toast.POSITION.TOP_LEFT,autoClose:8000});
        }
        else if(disp_r == 0 && (disp_q !== '--'))
        {
            toast.success('Your turn!!! ',{position: toast.POSITION.TOP_LEFT,autoClose:8000});
        }
    },[disp_r,disp_q]);
    
    if(props.SR)
    {
        props.C.subscribe("showR",{qos:0});
        props.C.on('message',(topic,msg)=>{
            if(topic === 'showR')
            {
                let res =JSON.parse(msg.toString());
                setdisp_r(res.showR);
            }
            
        });
    }
    else{
        props.C.unsubscribe("showR");
    }


    setInterval(() => {
        if(localStorage.getItem('info'))
        {
            let info = JSON.parse(localStorage.getItem('info'));
            setdisp_q(info.Q);
            setdisp_r(info.remain);
        }      
    }, 100);
  
    return (
        <div className="queue-container">
            <div className="queue-content">  
                <div className="Qnumber">
                    <h3 className="Qnumber_title">
                        Queue
                    </h3>
                    <h2>
                        {disp_q}
                    </h2>
                    
                </div>
                <div className="Qwaiting">
                        <h3>
                            Waiting
                        </h3>
                        <h2>
                        {disp_r}
                        </h2>
                </div>
            </div>
        </div>
    )
}

export default ShowQueue