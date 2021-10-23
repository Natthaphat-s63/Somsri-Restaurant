import './App.css';
import NavBar from './components/NavBar';
import Form from './components/FormSignUp';
import Addr from './components/Address';
function App() {
  return (
    <div className = 'wrapper'>
      <NavBar/>
      <Form/>
      <Addr/>
    </div>
  );
}

export default App;
