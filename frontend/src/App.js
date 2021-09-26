import React from 'react';
import './index.css';
import NavBar from './component/NavBar';
import Form from './component/Form'

function App(){
  return(
    <div className="App">
      <NavBar/>
      <div className="content">
        <Form/>
      </div>
    </div>
    
  );
}

export default App;
