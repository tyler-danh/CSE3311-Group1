import { useLocation, useNavigate } from 'react-router-dom'
import Logo from '../components/Logo'
import BackButton from '../components/BackButton'

function EncodeResult() {
  const location = useLocation()
  const navigate = useNavigate()
  const { fileUrl, fileName } = location.state || {}

  if (!fileUrl) {
    navigate('/')
    return null
  }

  const handleSave = () => {
    const a = document.createElement('a')
    a.href = fileUrl
    a.download = fileName || 'encoded.png'
    document.body.appendChild(a)
    a.click()
    document.body.removeChild(a)
  }

  return (
    <div className="container">
      <BackButton />
      <Logo />
      
      <h2 className="title">Encoded File</h2>
      
      <div className="result-files">
        <div className="file-display">{fileName || 'Encoded.zip'}</div>
      </div>

      <button className="button primary" onClick={handleSave}>
        Save
      </button>
    </div>
  )
}

export default EncodeResult
