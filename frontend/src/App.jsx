import { BrowserRouter as Router, Routes, Route } from 'react-router-dom'
import Home from './pages/Home'
import Encode from './pages/Encode'
import Decode from './pages/Decode'
import EncodeResult from './pages/EncodeResult'
import DecodeResult from './pages/DecodeResult'

function App() {
  return (
    <Router>
      <Routes>
        <Route path="/" element={<Home />} />
        <Route path="/encode" element={<Encode />} />
        <Route path="/decode" element={<Decode />} />
        <Route path="/encode-result" element={<EncodeResult />} />
        <Route path="/decode-result" element={<DecodeResult />} />
      </Routes>
    </Router>
  )
}

export default App
