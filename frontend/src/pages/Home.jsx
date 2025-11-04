import { useNavigate } from 'react-router-dom'
import Logo from '../components/Logo'

function Home() {
  const navigate = useNavigate()

  return (
    <div className="container">
      <Logo />
      <h2 className="title">Encode / Decode</h2>
      <button className="button" onClick={() => navigate('/encode')}>
        Select Carrier File
      </button>
    </div>
  )
}

export default Home
